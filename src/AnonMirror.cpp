/***
	AnonMirror / ps
***/


#include "cinder/app/AppBasic.h"
#include "cinder/imageio.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/TileRender.h"
#include "cinder/Text.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/Io.h"
#include "VOpenNIHeaders.h"
#include "VOpenNIDevice.h"
#include "VOpenNICommon.h"
#include "Resources.h"


using namespace ci;
using namespace ci::app;
using namespace std;

class ImageSourceKinectColor : public ImageSource 
{
public:
	ImageSourceKinectColor( uint8_t *buffer, int width, int height )
	: ImageSource(), mData( buffer ), _width(width), _height(height)
	{
		setSize( _width, _height );
		setColorModel( ImageIo::CM_RGB );
		setChannelOrder( ImageIo::RGB );
		setDataType( ImageIo::UINT8 );
	}
	
	~ImageSourceKinectColor()
	{
		// mData is actually a ref. It's released from the device. 
		/*if( mData ) {
			delete[] mData;
			mData = NULL;
		}*/
	}
	
	virtual void load( ImageTargetRef target )
	{
		ImageSource::RowFunc func = setupRowFunc( target );
		
		for( uint32_t row	 = 0; row < _height; ++row )
			((*this).*func)( target, row, mData + row * _width * 3 );
	}
	
protected:
	uint32_t					_width, _height;
	uint8_t						*mData;
};


class ImageSourceKinectDepth : public ImageSource 
{
public:
	ImageSourceKinectDepth( uint16_t *buffer, int width, int height )
	: ImageSource(), mData( buffer ), _width(width), _height(height)
	{
		setSize( _width, _height );
		setColorModel( ImageIo::CM_GRAY );
		setChannelOrder( ImageIo::Y );
		setDataType( ImageIo::UINT16 );
	}
	
	~ImageSourceKinectDepth()
	{
		// mData is actually a ref. It's released from the device. 
		/*if( mData ) {
		 delete[] mData;
		 mData = NULL;
		 }*/
	}
	
	virtual void load( ImageTargetRef target )
	{
		ImageSource::RowFunc func = setupRowFunc( target );
		
		for( uint32_t row = 0; row < _height; ++row )
			((*this).*func)( target, row, mData + row * _width );
	}
	
protected:
	uint32_t					_width, _height;
	uint16_t					*mData;
};


class AnonMirrorApp : public AppBasic, V::UserListener
{
public:
	static const int WIDTH = 1280;
	static const int HEIGHT = 720;

	static const int KINECT_COLOR_WIDTH = 640;	//1280;
	static const int KINECT_COLOR_HEIGHT = 480;	//1024;
	static const int KINECT_COLOR_FPS = 30;	//15;
	static const int KINECT_DEPTH_WIDTH = 640;
	static const int KINECT_DEPTH_HEIGHT = 480;
	static const int KINECT_DEPTH_FPS = 30;

	AnonMirrorApp();
	~AnonMirrorApp();
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
	void beepOn();
	void keyDown( KeyEvent event );
	
	void onNewUser( V::UserEvent event );
	void onLostUser( V::UserEvent event );

    void onUserExit( V::UserEvent event );
    void onUserReEnter( V::UserEvent event );
	
	ImageSourceRef getColorImage()
	{
		// register a reference to the active buffer
		uint8_t *activeColor = _device0->getColorMap();
		return ImageSourceRef( new ImageSourceKinectColor( activeColor, KINECT_COLOR_WIDTH, KINECT_COLOR_HEIGHT ) );
	}
	
	ImageSourceRef getUserImage( int id )
	{
		_device0->getLabelMap( id, pixels );
		return ImageSourceRef( new ImageSourceKinectDepth( pixels, KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT ) );
	}
	
	ImageSourceRef getDepthImage()
	{
		// register a reference to the active buffer
		uint16_t *activeDepth = _device0->getDepthMap();
		return ImageSourceRef( new ImageSourceKinectDepth( activeDepth, KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT ) );
	} 
	
	ImageSourceRef getDepthImage24()
	{
		// register a reference to the active buffer
		uint8_t *activeDepth = _device0->getDepthMap24();
		return ImageSourceRef( new ImageSourceKinectColor( activeDepth, KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT ) );
	}
	
	void prepareSettings( Settings *settings );
	
public:	// Members
	
	V::OpenNIDeviceManager*	_manager;
	V::OpenNIDevice::Ref	_device0;

	gl::Texture				mColorTex;
	gl::Texture				mDepthTex;
	gl::Texture				mOneUserTex;	
	gl::Texture				mTwoUserTex;	
	gl::Texture				mThreeUserTex;	
	gl::Texture				mFourUserTex;	
	gl::Texture				mFiveUserTex;	
	
	gl::Texture				mLaughingManTex;
	gl::Texture				mAnonTex;
	gl::Texture				mAnonAlphaTex;
	gl::Texture				mAnonLogo;

	//std::string				mLabel;
	std::vector <gl::Texture *> mLabel;
	
	//gl::Texture		mTexture;	
	gl::GlslProg	mShader;
	gl::GlslProg	mShaderFace;

	uint16_t* pixels;

	audio::SourceRef mTone;
	std::vector <audio::SourceRef> mNoise;	
	std::vector <audio::SourceRef> mTTS;
	
	double currTime;
	
	bool qNoise;
	double qNoiseTime;
	double lNoiseTime;

	bool qTone;
	double qToneTime;
	double lToneTime;
	
	bool english;
	
	int mLineNumber;

	bool qText;
	double qTextTime;
	double lTextTime;
	double qTextLagTime;
	double lTextLagTime;
	
	bool captureScreen;
	double captureScreenTime;
	double captureScreenTimeLag;
};

AnonMirrorApp::AnonMirrorApp() {}
AnonMirrorApp::~AnonMirrorApp()
{
	if( pixels )
	{
		delete[] pixels;
		pixels = NULL;
	}
}

void AnonMirrorApp::prepareSettings( Settings *settings )
{
	settings->setFrameRate( 30 );
	settings->setWindowSize( WIDTH, HEIGHT );
	settings->setFullScreenSize( WIDTH, HEIGHT );
	settings->setTitle( "AnonMirror" );
	//settings->setShouldQuit( true );
	//settings->setFullScreen( true );
}


void AnonMirrorApp::setup()
{
	//
	// init kinect
	//
	V::OpenNIDeviceManager::USE_THREAD = false;
	_manager = V::OpenNIDeviceManager::InstancePtr();
	
	//_device0 = _manager->createDevice("../../../data/configIR.xml", true);
	//_device0 = _manager->createDevice( loadResource(CONFIGIR), true);
	//_device0 = _manager->createDevice(0,"/Users/filipecruz/Documents/AnonMirror/cinder_master/blocks/BlockOpenNI/samples/AnonMirror/data/configIR.xml", true);
	    
    //_manager->createDevices( 1, V::NODE_TYPE_IMAGE | V::NODE_TYPE_DEPTH | V::NODE_TYPE_USER );
    _manager->createDevices( 1, V::NODE_TYPE_IMAGE | V::NODE_TYPE_DEPTH | V::NODE_TYPE_SCENE | V::NODE_TYPE_USER );
	_device0 = _manager->getDevice( 0 );
    _device0->setDepthInvert( false );
    _device0->setDepthShiftMul( 4 );
    
	if( !_device0 ) 
	{
		DEBUG_MESSAGE( "(App)  Can't find a kinect device\n" );
        quit();
        shutdown();
	}
    _device0->addListener( this );
    
	if( !_device0 ) 
	{
		DEBUG_MESSAGE( "(App) Couldn't init device0\n" );
		exit( 0 );
	}

	_device0->setAlignWithDepthGenerator();
	_device0->setHistogram( true );	// Enable histogram depth map (RGB8bit bitmap)
	_manager->start();
//	_manager->mPrimaryGen = _device0->getDepthGenerator();
	//_manager->setNumMaxUsers(5);
    _device0->enableOneTimeCalibration( true );

	
	pixels = NULL;
	pixels = new uint16_t[ KINECT_DEPTH_WIDTH*KINECT_DEPTH_HEIGHT ];
	
	gl::Texture::Format format;
	gl::Texture::Format depthFormat;
	mColorTex = gl::Texture( KINECT_COLOR_WIDTH, KINECT_COLOR_HEIGHT, format );
	mDepthTex = gl::Texture( KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT, format );
	mOneUserTex = gl::Texture( KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT, format );
	mTwoUserTex = gl::Texture( KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT, format );
	mThreeUserTex = gl::Texture( KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT, format );
	mFourUserTex = gl::Texture( KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT, format );
	mFiveUserTex = gl::Texture( KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT, format );

	
	//
	// init text
	//
	
	std::vector<std::string> listStrings;
	listStrings.push_back("I am your Anonymous mirror");
	listStrings.push_back("See yourself in me");

	listStrings.push_back("I reflect the Anonymous in you");
	listStrings.push_back("You reflect our society");
	
	listStrings.push_back("I protect your identity");
	listStrings.push_back("You protect our society");
	
	listStrings.push_back("Change yourself");
	listStrings.push_back("Change society");
	
	listStrings.push_back("Are you hidden?");
	listStrings.push_back("Society is real!");

	listStrings.push_back("You are Anonymous");
	listStrings.push_back("You are Legion");

	listStrings.push_back("You do not forgive");
	listStrings.push_back("You do not forget");

	listStrings.push_back("Expect us");
	listStrings.push_back("We expect you");

	// translate .pt, starts at size()/2
	
	listStrings.push_back("Eu sou o teu espelho anónimo"); 
	listStrings.push_back("Vê-te no meu reflexo");
	
	listStrings.push_back("Eu reflicto o anónimo em ti");
	listStrings.push_back("Tu reflectes a nossa sociedade");
	
	listStrings.push_back("Eu protejo a tua identidade");
	listStrings.push_back("Tu proteges a nossa sociedade");
	
	listStrings.push_back("Muda-te");
	listStrings.push_back("Muda a sociedade");
	
	listStrings.push_back("Andas escondido?");
	listStrings.push_back("A sociedade é real!");
	
	listStrings.push_back("Tu és anónimo");
	listStrings.push_back("Tu és parte de nós");
	
	listStrings.push_back("Nunca perdoas");
	listStrings.push_back("Nunca esqueces");
	
	listStrings.push_back("Aguarda-nos");
	listStrings.push_back("Nós esperamos-te");
	
	std::vector<std::string>::iterator it;
	
	for ( it=listStrings.begin() ; it < listStrings.end(); it++ )
	{
		TextLayout layout;
		layout.setFont( Font( "HelveticaNeue",  38 ) );
		layout.setColor( Color( 1, 1, 1 ) );
		layout.addCenteredLine( *it );
		gl::Texture *mLabelTex = new gl::Texture( layout.render( true ) );
		mLabel.push_back(mLabelTex);
	}
	
	english = false;
	mLineNumber = -1; //starting line
	lTextLagTime = 2.5; //how long it takes for next line to be shown
	lTextTime = 4.0; //how long the line stays on screen

	// demo vocals from http://www2.research.att.com/~ttsweb/tts/demo.php
	
	
	
	//
	// load face textures
	//
	
	try {
		mAnonTex = gl::Texture( loadImage( loadResource( RES_ANON ) ) );
	}
	catch( ... ) {
		std::cout << "Unable to load image" << std::endl;
	}

	try {
		mAnonAlphaTex = gl::Texture( loadImage( loadResource( RES_ANON_ALPHA ) ) );
	}
	catch( ... ) {
		std::cout << "Unable to load image" << std::endl;
	}
	
	try {
		mAnonLogo = gl::Texture( loadImage( loadResource( RES_ANON_LOGO ) ) );
	}
	catch( ... ) {
		std::cout << "Unable to load image" << std::endl;
	}
	
	//
	// init shaders
	//
	
	try {
		mShader = gl::GlslProg( loadResource( RES_PASSTHRU_VERT ), loadResource( RES_ANONMIRROR_FRAG ) );
	}
	catch( gl::GlslProgCompileExc &exc ) {
		std::cout << "Shader compile error: " << std::endl;
		std::cout << exc.what();
	}
	catch( ... ) {
		std::cout << "Unable to load shader" << std::endl;
	}
	
	try {
		mShaderFace = gl::GlslProg( loadResource( RES_PASSTHRU_VERT ), loadResource( RES_ANONMIRROR_FACE_FRAG ) );
	}
	catch( gl::GlslProgCompileExc &exc ) {
		std::cout << "Shader compile error: " << std::endl;
		std::cout << exc.what();
	}
	catch( ... ) {
		std::cout << "Unable to load shader" << std::endl;
	}
	
	//
	// load and init sfx
	//
	
	mTone = audio::load( loadResource( RES_TONE ) );

	mNoise.push_back(audio::load( loadResource( RES_NOISE1 ) ));
	mNoise.push_back(audio::load( loadResource( RES_NOISE2 ) ));
	mNoise.push_back(audio::load( loadResource( RES_NOISE3 ) ));
	mNoise.push_back(audio::load( loadResource( RES_NOISE4 ) ));
	mNoise.push_back(audio::load( loadResource( RES_NOISE5 ) ));

	mTTS.push_back(audio::load( loadResource( RES_TTS01 ) ));
	mTTS.push_back(audio::load( loadResource( RES_TTS02 ) ));
	mTTS.push_back(audio::load( loadResource( RES_TTS03 ) ));
	mTTS.push_back(audio::load( loadResource( RES_TTS04 ) ));
	mTTS.push_back(audio::load( loadResource( RES_TTS05 ) ));
	mTTS.push_back(audio::load( loadResource( RES_TTS06 ) ));
	mTTS.push_back(audio::load( loadResource( RES_TTS07 ) ));
	mTTS.push_back(audio::load( loadResource( RES_TTS08 ) ));
	mTTS.push_back(audio::load( loadResource( RES_TTS09 ) ));
	mTTS.push_back(audio::load( loadResource( RES_TTS10 ) ));
	mTTS.push_back(audio::load( loadResource( RES_TTS11 ) ));
	mTTS.push_back(audio::load( loadResource( RES_TTS12 ) ));
	mTTS.push_back(audio::load( loadResource( RES_TTS13 ) ));
	mTTS.push_back(audio::load( loadResource( RES_TTS14 ) ));
	mTTS.push_back(audio::load( loadResource( RES_TTS15 ) ));
	mTTS.push_back(audio::load( loadResource( RES_TTS16 ) ));

	lNoiseTime = 0.5;
	lToneTime = 2.20;

	qNoise = false;
	qTone = false;
	
	captureScreen = false;
	captureScreenTimeLag = 5.0;

}

void AnonMirrorApp::mouseDown( MouseEvent event )
{
}

void AnonMirrorApp::update()
{	
	_manager->update();

	// Update textures
	mColorTex.update( getColorImage() );
	mDepthTex.update( getDepthImage24() );

    if( _manager->hasUsers() ) mOneUserTex.update( getUserImage(0) ); //0 gets all
/*
	if( _manager->hasUsers() ) mOneUserTex.update( getUserImage(1) ); //0 gets all
	if( _manager->hasUsers() ) mTwoUserTex.update( getUserImage(2) ); //0 gets all
	if( _manager->hasUsers() ) mThreeUserTex.update( getUserImage(3) ); //0 gets all
	if( _manager->hasUsers() ) mFourUserTex.update( getUserImage(4) ); //0 gets all
	if( _manager->hasUsers() ) mFiveUserTex.update( getUserImage(5) ); //0 gets all
*/
	
	if (qNoise) {
		if (getElapsedSeconds() > qNoiseTime + lNoiseTime ) qNoise = false;
	}
	
	if (qTone) {
		if (getElapsedSeconds() > qToneTime + lToneTime ) qTone = false;
	}
	
	if (qText) {
		if (getElapsedSeconds() > qTextTime + lTextTime ) {
			qText = false;
			qTextLagTime = getElapsedSeconds();
		}
	} else {
		if (getElapsedSeconds() > qTextLagTime + lTextLagTime ) {
			qText = true;
			qTextTime = getElapsedSeconds();
			mLineNumber++;
			if (mLineNumber >= mLabel.size()/2) {
				mLineNumber = -1;
				beepOn();
				qText = false;
				qTextLagTime = getElapsedSeconds();
			} else {
				audio::Output::play( mTTS.at(mLineNumber) );
			}
		}
	}
	
/*	if (captureScreen) {
		if (getElapsedSeconds() > captureScreenTime + captureScreenTimeLag ) {
			stringstream some_stream;
			some_stream << "image_";
			some_stream << getElapsedFrames();
			some_stream << ".png";
			string some_string = some_stream.str();
			writeImage( some_string, copyWindowSurface() );
			captureScreenTime = getElapsedSeconds();
		}
	}*/
	
}

void AnonMirrorApp::draw()
{
	
	if (qTone) {
		gl::clear( Color( 1.0, 1.0, 1.0 ), true ); 
		//gl::pushModelView();
		glEnable(GL_TEXTURE_2D);
		gl::draw( mAnonLogo, Vec2f(getWindowWidth()*.5-mAnonLogo.getWidth()*.5, getWindowHeight()*.5-mAnonLogo.getHeight()*.5) );
		//mAnonLogo.disable();
		//gl::popModelView();
	} else {
	
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ), true ); 

	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );

	if( _manager->hasUsers() ) {

		mShader.bind();
		mShader.uniform( "tex0", 0 );
		mShader.uniform( "tex1", 1 );
		mShader.uniform( "tex2", 2 );
		/*mShader.uniform( "tex3", 3 );
		mShader.uniform( "tex4", 4 );
		mShader.uniform( "tex5", 5 );
		mShader.uniform( "tex6", 6 );*/
		mShader.uniform( "sampleOffset", Vec2f( cos( getElapsedSeconds() ), sin( getElapsedSeconds() ) ) * ( 3.0f / getWindowWidth() ) );
		mColorTex.bind( 0 );
		mDepthTex.bind( 1 );
		mOneUserTex.bind( 2 );
		/*mTwoUserTex.bind( 3 );
		mThreeUserTex.bind( 4 );
		mFourUserTex.bind( 5 );
		mFiveUserTex.bind( 6 );*/
		gl::drawSolidRect( getWindowBounds() );
		mShader.unbind();
	} else {
		gl::draw( mColorTex, getWindowBounds() );
		//gl::draw( mDepthTex, Rectf( xoff, yoff, xoff+sx, yoff+sy) );
		//gl::draw( mColorTex, Rectf( xoff+sx*1, yoff, xoff+sx*2, yoff+sy) );
	}

/*	glEnable( GL_TEXTURE_2D );
	gl::draw( mDepthTex, Rectf( 0, 0, 320, 240) );
	gl::draw( mColorTex, Rectf( 320, 0, 320+320, 240) );
	gl::draw( mOneUserTex, Rectf( 640, 0, 640+320, 240) );
	gl::draw( mTwoUserTex, Rectf( 0, 240, 320, 480) );
*/
		
	//
	// draw face masks
	//
		
	if( _manager->hasUsers() )// && _manager->hasUser(1) )
	{
		
		glEnable( GL_TEXTURE_2D );
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		//for (int i = 1; i < _manager->getNumOfUsers()+1; i++) {
		//for (int i = 1; i < 32; i++) {
        V::OpenNIUserList mUserList = _manager->getUserList();
        for( V::OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); ++it )
            {
                //console() << "User " << (*it)->getId() << " state: " << (*it)->getUserState()  << std::endl;
                
                //int i = (*it)->getId();
                //if ( _manager->hasUser(i) ) {
				//if (_manager->getUser(i)->getUserState() == V::USER_TRACKING) {
				//V::OpenNIBoneList mBoneList = _manager->getUser(i)->getBoneList();
				V::OpenNIBoneList mBoneList = (*it)->getBoneList();
				
                V::OpenNIBone* head = mBoneList[0];
		
				if (head->positionProjective[0] != 0) { 
				XnPoint3D point;
				point.X = head->positionProjective[0] * 0.0015625f * getWindowWidth();
				point.Y = head->positionProjective[1] * 0.00208333333333333333333333333333f * getWindowHeight();
			
				mShaderFace.bind();
				mShaderFace.uniform( "tex0", 0 );
				mShaderFace.uniform( "tex1", 1 );
				mAnonTex.bind( 0 );
				mAnonAlphaTex.bind( 1 );
				gl::drawSolidRect( Rectf( point.X - 20, point.Y - 30, point.X + 30, point.Y + 20) );
				mShaderFace.unbind(); 
				//}
                //}
			}
		}
		glDisable(GL_BLEND);
	}

	if (qText) {

		int starterpos = 0;
		if (!english) starterpos = mLabel.size()/2;
		gl::Texture *mLabelTex = mLabel.at(starterpos + mLineNumber);
		
		glColor3f(0.0, 0.0, 0.0);
		int borderSize = 15;
		int bottomTextPad = 80;
		gl::drawSolidRect( Rectf( 
								 (getWindowWidth() - mLabelTex->getWidth()) / 2 - borderSize,
								 getWindowHeight() - bottomTextPad - borderSize, 
								 (getWindowWidth() + mLabelTex->getWidth()) / 2 + borderSize,
								 getWindowHeight() - bottomTextPad + mLabelTex->getHeight() + borderSize
								)
		);

		glColor3f(1.0, 1.0, 1.0);
		//glEnable( GL_TEXTURE_2D );
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		gl::pushModelView();
		gl::draw( *mLabelTex, Vec2f( (getWindowWidth() - mLabelTex->getWidth()) / 2 , getWindowHeight() - bottomTextPad) );
		mLabelTex->disable();
		gl::popModelView();
		glDisable(GL_BLEND);
	}
		
	}
}


void AnonMirrorApp::beepOn() {
	audio::Output::play( mTone );
	qTone = true;
	qToneTime = getElapsedSeconds();
}

// todo: debug fullscreen, debug crash on exit

void AnonMirrorApp::keyDown( KeyEvent event )
{
	switch( event.getChar() ) {
		case 'f':
			setFullScreen( ! isFullScreen() );
			break;
		//case 'a':
		//	audio::Output::play( mAudioSource );
		//	break;
		case 'n':
			audio::Output::play( mNoise.at(1) );
			qNoise = true;
			qNoiseTime = getElapsedSeconds();
			break;
		case 'b':
			beepOn();
			break;
		case 'l':
			english = !english;
			break;
		case 's':
			/*stringstream some_stream;
			some_stream << "image_";
			some_stream << getElapsedFrames();
			some_stream << ".png";
			string some_string = some_stream.str();
			writeImage( some_string, copyWindowSurface() );*/
			captureScreen = !captureScreen;
			captureScreenTime = getElapsedSeconds();
			break;
	}
	
	if( event.getCode() == KeyEvent::KEY_ESCAPE )
	{
		if (isFullScreen()) setFullScreen( !isFullScreen() );
		this->quit();
		this->shutdown();
	}
}


void AnonMirrorApp::onNewUser( V::UserEvent event )
{
    app::console() << " :::: User Added With ID: " << event.mId << std::endl;
    //mUsersTexMap.insert( std::make_pair( event.mId, gl::Texture(KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT) ) );
}


void AnonMirrorApp::onLostUser( V::UserEvent event )
{
    app::console() << " :::: User Lost With ID: " << event.mId << std::endl;
    //mUsersTexMap.erase( event.mId );
}


void AnonMirrorApp::onUserExit( V::UserEvent event )
{
    app::console() << " :::: User Exit With ID: " << event.mId << std::endl;
    //mUsersTexMap.erase( event.mId );
}

void AnonMirrorApp::onUserReEnter( V::UserEvent event )
{
    app::console() << " :::: User ReEnter With ID: " << event.mId << std::endl;
    //mUsersTexMap.erase( event.mId );
}



CINDER_APP_BASIC( AnonMirrorApp, RendererGl )
