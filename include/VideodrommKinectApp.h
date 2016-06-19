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
};
