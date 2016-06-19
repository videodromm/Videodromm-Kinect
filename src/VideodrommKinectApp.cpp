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
	//mParams.draw();
	getWindow()->setTitle("(" + mVDSettings->sFps + " fps) " + toString(mVDSettings->iBeat) + " Videodromm");
}


CINDER_APP(VideodrommKinectApp, RendererGl)
