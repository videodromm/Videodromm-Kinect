#include "VideodrommKinectApp.h"

void VideodrommKinectApp::setup()
{
	// Settings
	mVDSettings = VDSettings::create();
	// Session
	mVDSession = VDSession::create(mVDSettings);
	// Animation
	mVDAnimation = VDAnimation::create(mVDSettings, mVDSession);
	// initialize 
#if (defined(  CINDER_MSW) )
	mTexturesFilepath = getAssetPath("") / "defaulttextures.xml";
#else
	mTexturesFilepath = getAssetPath("") / "defaulttexturesquicktime.xml";
#endif
	if (fs::exists(mTexturesFilepath)) {
		// load textures from file if one exists
		mTexs = VDTexture::readSettings(mVDAnimation, loadFile(mTexturesFilepath));
	}
	else {
		// otherwise create a texture from scratch
		mTexs.push_back(TextureAudio::create(mVDAnimation));
	}
	glEnable(GL_TEXTURE_2D);
	gl::enableAlphaBlending();
	gl::enableAdditiveBlending();

	mRgbScale = 50;
	mDenScale = 50;

	mFluid2D.set(192, 192);
	mFluid2D.setDensityDissipation(0.99f);
	mFluid2D.setRgbDissipation(0.99f);
	mVelScale = 3.0f*std::max(mFluid2D.resX(), mFluid2D.resY());
	mFluid2D.setDt(0.1f);
	mFluid2D.enableDensity();
	mFluid2D.enableRgb();
	mFluid2D.enableVorticityConfinement();

	mParticles.setup(getWindowBounds(), &mFluid2D);
	// kinec
	Kinect::DeviceType type = Kinect::Kinect1;
#ifdef KINECT_V2
	type = Kinect::Kinect2;
#endif
	mDevice = Kinect::Device::create(type);
	if (!mDevice->isValid())
	{
		quit();
		return;
	}
	mDevice->signalDepthDirty.connect(std::bind(&VideodrommKinectApp::updateDepthRelated, this));

	mDepthW = mDevice->getDepthSize().x;
	mDepthH = mDevice->getDepthSize().y;
	mDiffMat = cv::Mat1b(mDepthH, mDepthW);
	mDiffChannel = Channel(mDepthW, mDepthH, mDiffMat.step, 1, mDiffMat.ptr());

}
void VideodrommKinectApp::updateDepthRelated()
{
	updateTexture(mDepthTexture, mDevice->depthChannel);


	mDiffMat.setTo(cv::Scalar::all(0));
	//float x0 = corners[CORNER_DEPTH_LT].x - depthOrigin.x;
	//float x1 = corners[CORNER_DEPTH_RB].x - depthOrigin.x;
	//float y0 = corners[CORNER_DEPTH_LT].y - depthOrigin.y;
	//float y1 = corners[CORNER_DEPTH_RB].y - depthOrigin.y;

	int cx = CENTER_X * mDepthW;
	int cy = CENTER_Y * mDepthH;
	int radius = RADIUS * mDepthH;
	int radius_sq = radius * radius;

	for (int yy = mInputRoi.y1; yy < mInputRoi.y2; yy++)
	{
		// TODO: cache row pointer
		int y = yy;
		for (int xx = mInputRoi.x1; xx < mInputRoi.x2; xx++)
		{
			int x = LEFT_RIGHT_FLIPPED ? (mDepthW - xx) : xx;
			uint16_t bg = *mBackChannel.getData(x, y);
			uint16_t dep = *mDevice->depthChannel.getData(x, y);
			if (dep > 0 && bg - dep > MIN_THRESHOLD_MM && bg - dep < MAX_THRESHOLD_MM)
			{
				// TODO: optimize
				if (!CIRCLE_MASK_ENABLED || (cx - x) * (cx - x) + (cy - y) * (cy - y) < radius_sq)
				{
					mDiffMat(yy, xx) = 255;
				}
			}
		}
	}

	if (TRACKING_SMOOTH > 0)
	{
		cv::Mat element = getStructuringElement(cv::MORPH_RECT, cv::Size(TRACKING_SMOOTH * 2 + 1, TRACKING_SMOOTH * 2 + 1),
			cv::Point(TRACKING_SMOOTH, TRACKING_SMOOTH));
		cv::morphologyEx(mDiffMat, mDiffMat, cv::MORPH_OPEN, element);
	}

	updateTexture(mDiffTexture, mDiffChannel);
	std::vector<Blob> blobs;
	BlobFinder::Option option;
	option.minArea = MIN_AREA;
	option.handOnlyMode = FINGER_MODE_ENABLED;
	option.handDistance = FINGER_SIZE;
	BlobFinder::execute(mDiffMat, blobs, option);
	mBlobTracker.trackBlobs(blobs);
	//sendTuioMessage(*mOscSender, mBlobTracker);
}
void VideodrommKinectApp::fileDrop(FileDropEvent event)
{
	int index = 1;
	string ext = "";
	// use the last of the dropped files
	boost::filesystem::path mPath = event.getFile(event.getNumFiles() - 1);
	string mFile = mPath.string();
	int dotIndex = mFile.find_last_of(".");
	int slashIndex = mFile.find_last_of("\\");
	bool found = false;

	if (dotIndex != std::string::npos && dotIndex > slashIndex) ext = mFile.substr(mFile.find_last_of(".") + 1);

	if (ext == "wav" || ext == "mp3")
	{
		for (auto tex : mTexs)
		{
			if (!found) {
				if (tex->getType() == VDTexture::AUDIO) {
					tex->loadFromFullPath(mFile);
					found = true;
				}
			}
		}
	}
	else if (ext == "png" || ext == "jpg")
	{
		for (auto tex : mTexs)
		{
			if (!found) {
				if (tex->getType() == VDTexture::IMAGE) {
					tex->loadFromFullPath(mFile);
					found = true;
				}
			}
		}
	}
	else if (ext == "mov")
	{
		for (auto tex : mTexs)
		{
			if (!found) {
				if (tex->getType() == VDTexture::MOVIE) {
					tex->loadFromFullPath(mFile);
					found = true;
				}
			}
		}
	}
	else if (ext == "")
	{
		// try loading image sequence from dir
		for (auto tex : mTexs)
		{
			if (!found) {
				if (tex->getType() == VDTexture::IMAGESEQUENCE) {
					tex->loadFromFullPath(mFile);
					found = true;
				}
			}
		}
	}

}

void VideodrommKinectApp::cleanup()
{
	// save textures
	VDTexture::writeSettings(mTexs, writeFile(mTexturesFilepath));

	quit();
}
void VideodrommKinectApp::keyDown(KeyEvent event)
{
	switch (event.getCode()) {
	case KeyEvent::KEY_r:
		mFluid2D.initSimData();
		break;
	}
}
void VideodrommKinectApp::mouseDown(MouseEvent event)
{
	for (auto tex : mTexs)
	{
		tex->setXLeft(event.getX());
		tex->setYTop(event.getY());
	}
	mPrevPos = event.getPos();
	mColor.r = Rand::randFloat(0.25f, 1.0f);
	mColor.g = Rand::randFloat(0.25f, 1.0f);
	mColor.b = Rand::randFloat(0.25f, 1.0f);
}
void VideodrommKinectApp::mouseDrag(MouseEvent event)
{
	for (auto tex : mTexs)
	{
		tex->setXRight(event.getX());
		tex->setYBottom(event.getY());
	}
	float x = (event.getX() / (float)getWindowWidth())*mFluid2D.resX();
	float y = (event.getY() / (float)getWindowHeight())*mFluid2D.resY();
	float s = 10;

	if (event.isLeftDown()) {
		vec2 dv = vec2(event.getPos()) - mPrevPos;
		mFluid2D.splatVelocity(x, y, mVelScale*dv);
		mFluid2D.splatRgb(x, y, mRgbScale*mColor);
		if (mFluid2D.isBuoyancyEnabled()) {
			mFluid2D.splatDensity(x, y, mDenScale);
		}
		//
		for (int i = 0; i < 10; ++i) {
			vec2 partPos = vec2(event.getPos()) + vec2(Rand::randFloat(-s, s), Rand::randFloat(-s, s));
			float life = Rand::randFloat(2.0f, 4.0f);
			mParticles.append(Particle(partPos, life, mColor));
		}
	}

	mPrevPos = event.getPos();
}
void VideodrommKinectApp::update()
{
	mFluid2D.step();
	mParticles.update();
	mVDSettings->iFps = getAverageFps();
	mVDSettings->sFps = toString(floor(mVDSettings->iFps));

}
void VideodrommKinectApp::draw()
{
	gl::clear( Color::black() );
	i = 0;
	for (auto tex : mTexs)
	{
		int x = 128 * i;
		gl::draw(tex->getTexture(), Rectf(0 + x, 0, 128 + x, 128));
		i++;
	}

	gl::color(ColorAf(1.0f, 1.0f, 1.0f, 0.999f));
	float* data = const_cast<float*>((float*)mFluid2D.rgb().data());
	Surface32f surf(data, mFluid2D.resX(), mFluid2D.resY(), mFluid2D.resX()*sizeof(Colorf), SurfaceChannelOrder::RGB);


	if (!mTex) {
		mTex = gl::Texture::create(surf);
	}
	else {
		mTex->update(surf);
	}
	gl::draw(mTex, getWindowBounds());
	mTex->unbind();
	mParticles.draw();
	if (mDepthTexture)
	{
		//gl::ScopedGlslProg prog(mShader);
		gl::draw(mDepthTexture);

	}
	//visualizeBlobs(mBlobTracker);
	//mParams.draw();
	getWindow()->setTitle("(" + mVDSettings->sFps + " fps) " + toString(mVDSettings->iBeat) + " Videodromm");
}


CINDER_APP(VideodrommKinectApp, RendererGl)
