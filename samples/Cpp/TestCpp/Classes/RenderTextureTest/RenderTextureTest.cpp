#include "CCConfiguration.h"
#include "RenderTextureTest.h"
#include "../testBasic.h"

// Test #1 by Jason Booth (slipster216)
// Test #3 by David Deaco (ddeaco)


static std::function<Layer*()> createFunctions[] = {
    CL(RenderTextureSave),
    CL(RenderTextureIssue937),
    CL(RenderTextureZbuffer),
    CL(RenderTextureTestDepthStencil),
    CL(RenderTextureTargetNode),
    CL(SpriteRenderTextureBug),
};

#define MAX_LAYER   (sizeof(createFunctions)/sizeof(createFunctions[0]))
static int sceneIdx = -1; 

static Layer* nextTestCase()
{
    sceneIdx++;
    sceneIdx = sceneIdx % MAX_LAYER;

    Layer* layer = (createFunctions[sceneIdx])();
    layer->autorelease();

    return layer;
}

static Layer* backTestCase()
{
    sceneIdx--;
    int total = MAX_LAYER;
    if( sceneIdx < 0 )
        sceneIdx += total;    

    Layer* layer = (createFunctions[sceneIdx])();
    layer->autorelease();

    return layer;
}

static Layer* restartTestCase()
{
    Layer* layer = (createFunctions[sceneIdx])();
    layer->autorelease();

    return layer;
}

void RenderTextureTest::onEnter()
{
    BaseTest::onEnter();
}

void RenderTextureTest::restartCallback(Object* pSender)
{
    Scene* s = new RenderTextureScene();
    s->addChild(restartTestCase()); 

    Director::getInstance()->replaceScene(s);
    s->release();
}

void RenderTextureTest::nextCallback(Object* pSender)
{
    Scene* s = new RenderTextureScene();
    s->addChild( nextTestCase() );
    Director::getInstance()->replaceScene(s);
    s->release();
}

void RenderTextureTest::backCallback(Object* pSender)
{
    Scene* s = new RenderTextureScene();
    s->addChild( backTestCase() );
    Director::getInstance()->replaceScene(s);
    s->release();
} 

std::string RenderTextureTest::title()
{
    return "No title";
}

std::string RenderTextureTest::subtitle()
{
    return "";
}

/**
* Impelmentation of RenderTextureSave
*/
RenderTextureSave::RenderTextureSave()
{
    Size s = Director::getInstance()->getWinSize();

    // create a render texture, this is what we are going to draw into
    _target = RenderTexture::create(s.width, s.height, kTexture2DPixelFormat_RGBA8888);
    _target->retain();
    _target->setPosition(Point(s.width / 2, s.height / 2));

    // note that the render texture is a Node, and contains a sprite of its texture for convience,
    // so we can just parent it to the scene like any other Node
    this->addChild(_target, -1);

    // create a brush image to draw into the texture with
    _brush = Sprite::create("Images/fire.png");
    _brush->retain();
    _brush->setColor(Color3B::RED);
    _brush->setOpacity(20);
    this->setTouchEnabled(true);

    // Save Image menu
    MenuItemFont::setFontSize(16);
    MenuItem *item1 = MenuItemFont::create("Save Image", CC_CALLBACK_1(RenderTextureSave::saveImage, this));
    MenuItem *item2 = MenuItemFont::create("Clear", CC_CALLBACK_1(RenderTextureSave::clearImage, this));
    Menu *menu = Menu::create(item1, item2, NULL);
    this->addChild(menu);
    menu->alignItemsVertically();
    menu->setPosition(Point(VisibleRect::rightTop().x - 80, VisibleRect::rightTop().y - 30));
}

string RenderTextureSave::title()
{
    return "Touch the screen";
}

string RenderTextureSave::subtitle()
{
    return "Press 'Save Image' to create an snapshot of the render texture";
}

void RenderTextureSave::clearImage(cocos2d::Object *pSender)
{
    _target->clear(CCRANDOM_0_1(), CCRANDOM_0_1(), CCRANDOM_0_1(), CCRANDOM_0_1());
}

void RenderTextureSave::saveImage(cocos2d::Object *pSender)
{
    static int counter = 0;

    char png[20];
    sprintf(png, "image-%d.png", counter);
    char jpg[20];
    sprintf(jpg, "image-%d.jpg", counter);

    _target->saveToFile(png, kImageFormatPNG);
    _target->saveToFile(jpg, kImageFormatJPEG);
    

    Image *pImage = _target->newImage();

    Texture2D *tex = TextureCache::getInstance()->addUIImage(pImage, png);

    CC_SAFE_DELETE(pImage);

    Sprite *sprite = Sprite::createWithTexture(tex);

    sprite->setScale(0.3f);
    addChild(sprite);
    sprite->setPosition(Point(40, 40));
    sprite->setRotation(counter * 3);

    CCLOG("Image saved %s and %s", png, jpg);

    counter++;
}

RenderTextureSave::~RenderTextureSave()
{
    _brush->release();
    _target->release();
    TextureCache::getInstance()->removeUnusedTextures();
}

void RenderTextureSave::ccTouchesMoved(Set* touches, Event* event)
{
    Touch *touch = static_cast<Touch*>( touches->anyObject() );
    Point start = touch->getLocation();
    Point end = touch->getPreviousLocation();

    // begin drawing to the render texture
    _target->begin();

    // for extra points, we'll draw this smoothly from the last position and vary the sprite's
    // scale/rotation/offset
    float distance = start.getDistance(end);
    if (distance > 1)
    {
        int d = (int)distance;
        for (int i = 0; i < d; i++)
        {
            float difx = end.x - start.x;
            float dify = end.y - start.y;
            float delta = (float)i / distance;
            _brush->setPosition(Point(start.x + (difx * delta), start.y + (dify * delta)));
            _brush->setRotation(rand() % 360);
            float r = (float)(rand() % 50 / 50.f) + 0.25f;
            _brush->setScale(r);
            /*_brush->setColor(Color3B(CCRANDOM_0_1() * 127 + 128, 255, 255));*/
            // Use CCRANDOM_0_1() will cause error when loading libtests.so on android, I don't know why.
            _brush->setColor(Color3B(rand() % 127 + 128, 255, 255));
            // Call visit to draw the brush, don't call draw..
            _brush->visit();
        }
    }

    // finish drawing and return context back to the screen
    _target->end();
}

/**
 * Impelmentation of RenderTextureIssue937
 */

RenderTextureIssue937::RenderTextureIssue937()
{
    /*
    *     1    2
    * A: A1   A2
    *
    * B: B1   B2
    *
    *  A1: premulti sprite
    *  A2: premulti render
    *
    *  B1: non-premulti sprite
    *  B2: non-premulti render
    */
    LayerColor *background = LayerColor::create(Color4B(200,200,200,255));
    addChild(background);

    Sprite *spr_premulti = Sprite::create("Images/fire.png");
    spr_premulti->setPosition(Point(16,48));

    Sprite *spr_nonpremulti = Sprite::create("Images/fire.png");
    spr_nonpremulti->setPosition(Point(16,16));


    
    
    /* A2 & B2 setup */
    RenderTexture *rend = RenderTexture::create(32, 64, kTexture2DPixelFormat_RGBA8888);

    if (NULL == rend)
    {
        return;
    }

    // It's possible to modify the RenderTexture blending function by
    //        [[rend sprite] setBlendFunc:(BlendFunc) {GL_ONE, GL_ONE_MINUS_SRC_ALPHA}];

    rend->begin();
    spr_premulti->visit();
    spr_nonpremulti->visit();
    rend->end(); 

    Size s = Director::getInstance()->getWinSize();

    /* A1: setup */
    spr_premulti->setPosition(Point(s.width/2-16, s.height/2+16));
    /* B1: setup */
    spr_nonpremulti->setPosition(Point(s.width/2-16, s.height/2-16));

    rend->setPosition(Point(s.width/2+16, s.height/2));

    addChild(spr_nonpremulti);
    addChild(spr_premulti);
    addChild(rend);
}

std::string RenderTextureIssue937::title()
{
    return "Testing issue #937";
}

std::string RenderTextureIssue937::subtitle()
{
    return "All images should be equal...";
}

void RenderTextureScene::runThisTest()
{
    Layer* layer = nextTestCase();
    addChild(layer);

    Director::getInstance()->replaceScene(this);
}

/**
* Impelmentation of RenderTextureZbuffer
*/

RenderTextureZbuffer::RenderTextureZbuffer()
{
    this->setTouchEnabled(true);
    Size size = Director::getInstance()->getWinSize();
    LabelTTF *label = LabelTTF::create("vertexZ = 50", "Marker Felt", 64);
    label->setPosition(Point(size.width / 2, size.height * 0.25f));
    this->addChild(label);

    LabelTTF *label2 = LabelTTF::create("vertexZ = 0", "Marker Felt", 64);
    label2->setPosition(Point(size.width / 2, size.height * 0.5f));
    this->addChild(label2);

    LabelTTF *label3 = LabelTTF::create("vertexZ = -50", "Marker Felt", 64);
    label3->setPosition(Point(size.width / 2, size.height * 0.75f));
    this->addChild(label3);

    label->setVertexZ(50);
    label2->setVertexZ(0);
    label3->setVertexZ(-50);

    SpriteFrameCache::getInstance()->addSpriteFramesWithFile("Images/bugs/circle.plist");
    mgr = SpriteBatchNode::create("Images/bugs/circle.png", 9);
    this->addChild(mgr);
    sp1 = Sprite::createWithSpriteFrameName("circle.png");
    sp2 = Sprite::createWithSpriteFrameName("circle.png");
    sp3 = Sprite::createWithSpriteFrameName("circle.png");
    sp4 = Sprite::createWithSpriteFrameName("circle.png");
    sp5 = Sprite::createWithSpriteFrameName("circle.png");
    sp6 = Sprite::createWithSpriteFrameName("circle.png");
    sp7 = Sprite::createWithSpriteFrameName("circle.png");
    sp8 = Sprite::createWithSpriteFrameName("circle.png");
    sp9 = Sprite::createWithSpriteFrameName("circle.png");

    mgr->addChild(sp1, 9);
    mgr->addChild(sp2, 8);
    mgr->addChild(sp3, 7);
    mgr->addChild(sp4, 6);
    mgr->addChild(sp5, 5);
    mgr->addChild(sp6, 4);
    mgr->addChild(sp7, 3);
    mgr->addChild(sp8, 2);
    mgr->addChild(sp9, 1);

    sp1->setVertexZ(400);
    sp2->setVertexZ(300);
    sp3->setVertexZ(200);
    sp4->setVertexZ(100);
    sp5->setVertexZ(0);
    sp6->setVertexZ(-100);
    sp7->setVertexZ(-200);
    sp8->setVertexZ(-300);
    sp9->setVertexZ(-400);

    sp9->setScale(2);
    sp9->setColor(Color3B::YELLOW);
}

string RenderTextureZbuffer::title()
{
    return "Testing Z Buffer in Render Texture";
}

string RenderTextureZbuffer::subtitle()
{
    return "Touch screen. It should be green";
}

void RenderTextureZbuffer::ccTouchesBegan(cocos2d::Set *touches, cocos2d::Event *event)
{

    for (auto &item: *touches)
    {
        Touch *touch = static_cast<Touch*>(item);
        Point location = touch->getLocation();

        sp1->setPosition(location);
        sp2->setPosition(location);
        sp3->setPosition(location);
        sp4->setPosition(location);
        sp5->setPosition(location);
        sp6->setPosition(location);
        sp7->setPosition(location);
        sp8->setPosition(location);
        sp9->setPosition(location);
    }
}

void RenderTextureZbuffer::ccTouchesMoved(Set* touches, Event* event)
{
    for (auto &item: *touches)
    {
        Touch *touch = static_cast<Touch*>(item);
        Point location = touch->getLocation();

        sp1->setPosition(location);
        sp2->setPosition(location);
        sp3->setPosition(location);
        sp4->setPosition(location);
        sp5->setPosition(location);
        sp6->setPosition(location);
        sp7->setPosition(location);
        sp8->setPosition(location);
        sp9->setPosition(location);
    }
}

void RenderTextureZbuffer::ccTouchesEnded(Set* touches, Event* event)
{
    this->renderScreenShot();
}

void RenderTextureZbuffer::renderScreenShot()
{
    RenderTexture *texture = RenderTexture::create(512, 512);
    if (NULL == texture)
    {
        return;
    }
    texture->setAnchorPoint(Point(0, 0));
    texture->begin();

    this->visit();

    texture->end();

    Sprite *sprite = Sprite::createWithTexture(texture->getSprite()->getTexture());

    sprite->setPosition(Point(256, 256));
    sprite->setOpacity(182);
    sprite->setFlipY(1);
    this->addChild(sprite, 999999);
    sprite->setColor(Color3B::GREEN);

    sprite->runAction(Sequence::create(FadeTo::create(2, 0),
                                          Hide::create(),
                                          NULL));
}

// RenderTextureTestDepthStencil

RenderTextureTestDepthStencil::RenderTextureTestDepthStencil()
{
    Size s = Director::getInstance()->getWinSize();

    Sprite *sprite = Sprite::create("Images/fire.png");
    sprite->setPosition(Point(s.width * 0.25f, 0));
    sprite->setScale(10);
    RenderTexture *rend = RenderTexture::create(s.width, s.height, kTexture2DPixelFormat_RGBA4444, GL_DEPTH24_STENCIL8);

    glStencilMask(0xFF);
    rend->beginWithClear(0, 0, 0, 0, 0, 0);

    //! mark sprite quad into stencil buffer
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glColorMask(0, 0, 0, 1);
    sprite->visit();

    //! move sprite half width and height, and draw only where not marked
    sprite->setPosition(sprite->getPosition() + Point(sprite->getContentSize().width * sprite->getScale(), sprite->getContentSize().height * sprite->getScale() * 0.5));
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glColorMask(1, 1, 1, 1);
    sprite->visit();

    rend->end();

    glDisable(GL_STENCIL_TEST);

    rend->setPosition(Point(s.width * 0.5f, s.height * 0.5f));

    this->addChild(rend);
}

std::string RenderTextureTestDepthStencil::title()
{
    return "Testing depthStencil attachment";
}

std::string RenderTextureTestDepthStencil::subtitle()
{
    return "Circle should be missing 1/4 of its region";
}

// RenderTextureTest
RenderTextureTargetNode::RenderTextureTargetNode()
{
    /*
	 *     1    2
	 * A: A1   A2
	 *
	 * B: B1   B2
	 *
	 *  A1: premulti sprite
	 *  A2: premulti render
	 *
	 *  B1: non-premulti sprite
	 *  B2: non-premulti render
	 */
    LayerColor *background = LayerColor::create(Color4B(40,40,40,255));
    addChild(background);
    
    // sprite 1
    sprite1 = Sprite::create("Images/fire.png");
    
    // sprite 2
    sprite2 = Sprite::create("Images/fire_rgba8888.pvr");
    
    Size s = Director::getInstance()->getWinSize();
    
    /* Create the render texture */
    RenderTexture *renderTexture = RenderTexture::create(s.width, s.height, kTexture2DPixelFormat_RGBA4444);
    this->renderTexture = renderTexture;
    
    renderTexture->setPosition(Point(s.width/2, s.height/2));
    //		[renderTexture setPosition:Point(s.width, s.height)];
    //		renderTexture.scale = 2;
    
    /* add the sprites to the render texture */
    renderTexture->addChild(sprite1);
    renderTexture->addChild(sprite2);
    renderTexture->setClearColor(Color4F(0, 0, 0, 0));
    renderTexture->setClearFlags(GL_COLOR_BUFFER_BIT);
    
    /* add the render texture to the scene */
    addChild(renderTexture);
    
    renderTexture->setAutoDraw(true);
    
    scheduleUpdate();
    
    // Toggle clear on / off
    MenuItemFont *item = MenuItemFont::create("Clear On/Off", CC_CALLBACK_1(RenderTextureTargetNode::touched, this));
    Menu *menu = Menu::create(item, NULL);
    addChild(menu);

    menu->setPosition(Point(s.width/2, s.height/2));
}

void RenderTextureTargetNode::touched(Object* sender)
{
    if (renderTexture->getClearFlags() == 0)
    {
        renderTexture->setClearFlags(GL_COLOR_BUFFER_BIT);
    }
    else
    {
        renderTexture->setClearFlags(0);
        renderTexture->setClearColor(Color4F( CCRANDOM_0_1(), CCRANDOM_0_1(), CCRANDOM_0_1(), 1));
    }
}

void RenderTextureTargetNode::update(float dt)
{
    static float time = 0;
    float r = 80;
    sprite1->setPosition(Point(cosf(time * 2) * r, sinf(time * 2) * r));
    sprite2->setPosition(Point(sinf(time * 2) * r, cosf(time * 2) * r));
    
    time += dt;
}

string RenderTextureTargetNode::title()
{
    return "Testing Render Target Node";
}

string RenderTextureTargetNode::subtitle()
{
    return "Sprites should be equal and move with each frame";
}

// SpriteRenderTextureBug

SpriteRenderTextureBug::SimpleSprite::SimpleSprite() : rt(NULL) {}

SpriteRenderTextureBug::SimpleSprite* SpriteRenderTextureBug::SimpleSprite::create(const char* filename, const Rect &rect)
{
    SimpleSprite *sprite = new SimpleSprite();
    if (sprite && sprite->initWithFile(filename, rect))
    {
        sprite->autorelease();
    }
    else
    {
        CC_SAFE_DELETE(sprite);
    }
    
    return sprite;
}

void SpriteRenderTextureBug::SimpleSprite::draw()
{
    if (rt == NULL)
    {
		Size s = Director::getInstance()->getWinSize();
        rt = new RenderTexture();
        rt->initWithWidthAndHeight(s.width, s.height, kTexture2DPixelFormat_RGBA8888);
	}
	rt->beginWithClear(0.0f, 0.0f, 0.0f, 1.0f);
	rt->end();
    
	CC_NODE_DRAW_SETUP();
    
	BlendFunc blend = getBlendFunc();
	ccGLBlendFunc(blend.src, blend.dst);
    
    ccGLBindTexture2D(getTexture()->getName());
    
	//
	// Attributes
	//
    
	ccGLEnableVertexAttribs(kVertexAttribFlag_PosColorTex);
    
#define kQuadSize sizeof(_quad.bl)
	long offset = (long)&_quad;
    
	// vertex
	int diff = offsetof( V3F_C4B_T2F, vertices);
	glVertexAttribPointer(kVertexAttrib_Position, 3, GL_FLOAT, GL_FALSE, kQuadSize, (void*) (offset + diff));
    
	// texCoods
	diff = offsetof( V3F_C4B_T2F, texCoords);
	glVertexAttribPointer(kVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, kQuadSize, (void*)(offset + diff));
    
	// color
	diff = offsetof( V3F_C4B_T2F, colors);
	glVertexAttribPointer(kVertexAttrib_Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, kQuadSize, (void*)(offset + diff));
    
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

SpriteRenderTextureBug::SpriteRenderTextureBug()
{
    setTouchEnabled(true);
    
    Size s = Director::getInstance()->getWinSize();
    addNewSpriteWithCoords(Point(s.width/2, s.height/2));
}

SpriteRenderTextureBug::SimpleSprite* SpriteRenderTextureBug::addNewSpriteWithCoords(const Point& p)
{
    int idx = CCRANDOM_0_1() * 1400 / 100;
	int x = (idx%5) * 85;
	int y = (idx/5) * 121;
    
    SpriteRenderTextureBug::SimpleSprite *sprite = SpriteRenderTextureBug::SimpleSprite::create("Images/grossini_dance_atlas.png",
                                                                                                Rect(x,y,85,121));
    addChild(sprite);
    
    sprite->setPosition(p);
    
	FiniteTimeAction *action = NULL;
	float rd = CCRANDOM_0_1();
    
	if (rd < 0.20)
        action = ScaleBy::create(3, 2);
	else if (rd < 0.40)
		action = RotateBy::create(3, 360);
	else if (rd < 0.60)
		action = Blink::create(1, 3);
	else if (rd < 0.8 )
		action = TintBy::create(2, 0, -255, -255);
	else
		action = FadeOut::create(2);
    
    FiniteTimeAction *action_back = action->reverse();
    Sequence *seq = Sequence::create(action, action_back, NULL);
    
    sprite->runAction(RepeatForever::create(seq));
    
    //return sprite;
    return NULL;
}

void SpriteRenderTextureBug::ccTouchesEnded(Set* touches, Event* event)
{
    for (auto &item: *touches)
    {
        Touch *touch = static_cast<Touch*>(item);
        Point location = touch->getLocation();
        addNewSpriteWithCoords(location);
    }
}

std::string SpriteRenderTextureBug::title()
{
    return "SpriteRenderTextureBug";
}

std::string SpriteRenderTextureBug::subtitle()
{
    return "Touch the screen. Sprite should appear on under the touch";
}
