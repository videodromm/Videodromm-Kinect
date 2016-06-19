#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/Rand.h"
//#include "cinder/params/Params.h"
// particles
#include "cinder/app/RendererGl.h"
#include "cinderfx/Fluid2D.h"
#include "Particles.h"
// kinec
#include "CinderOpenCV.h"
#include "BlobTracker.h"
#include "KinectDevice.h"
#include "TextureHelper.h"

// Settings
#include "VDSettings.h"
// Session
#include "VDSession.h"
// Animation
#include "VDAnimation.h"

#include "VDTexture.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace VideoDromm;

class VideodrommKinectApp : public App {

public:
	void						setup() override;
	void						mouseDown(MouseEvent event) override;
	void						mouseDrag(MouseEvent event) override;
	void						keyDown(KeyEvent event) override;
	void						update() override;
	void						draw() override;
	void						fileDrop(FileDropEvent event) override;
	void						cleanup() override;
	void						updateDepthRelated();
private:
	// Settings
	VDSettingsRef				mVDSettings;
	// Session
	VDSessionRef				mVDSession;
	// Log
	VDLogRef					mVDLog;
	// Animation
	VDAnimationRef				mVDAnimation;

	VDTextureList				mTexs;
	fs::path					mTexturesFilepath;
	int							i, x;
	// particles
	float						mVelScale;
	float						mDenScale;
	float						mRgbScale;
	ci::vec2					mPrevPos;
	cinderfx::Fluid2D			mFluid2D;
	ci::gl::Texture2dRef		mTex;
	//ci::params::InterfaceGl		mParams;
	//
	ParticleSystem				mParticles;
	ci::Colorf					mColor;
	std::map<int, ci::Colorf>	mTouchColors;
	// kinec
	Kinect::DeviceRef			mDevice;
	int							mDepthW, mDepthH;
	// vision
	Channel16u mBackChannel;
	gl::TextureRef mBackTexture;
	BlobTracker mBlobTracker;

	Channel mDiffChannel;
	cv::Mat1b mDiffMat;
	gl::TextureRef mDiffTexture;

	Rectf mInputRoi;
	Rectf mOutputMap;

	//constants
	const float CENTER_X = 0.5f;
	const float CENTER_Y = 0.5f;
	const float RADIUS = 0.5f;
	const bool LEFT_RIGHT_FLIPPED = false;
	const float MIN_THRESHOLD_MM = 50.0f;
	const float MAX_THRESHOLD_MM = 1000.0f;
	const float MIN_AREA = 100.0f;
	const bool CIRCLE_MASK_ENABLED = false;
	const float FINGER_SIZE = 30.0f;
	const int TRACKING_SMOOTH = 2;
	const bool FINGER_MODE_ENABLED = false;
	gl::TextureRef mDepthTexture;
};
