//
//  HelloWorldLayer.m
//  EAGLViewBug
//
//  Created by Wylan Werth on 7/5/10.
//  Copyright BanditBear Games 2010. All rights reserved.
//

// Import the interfaces
#include"Bug-914.h"

Scene* Bug914Layer::scene()
{
    // 'scene' is an autorelease object.
    Scene *scene = Scene::create();
    // 'layer' is an autorelease object.
    Bug914Layer* layer = Bug914Layer::create();

    // add layer as a child to scene
    scene->addChild(layer);

    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool Bug914Layer::init()
{
    // always call "super" init
    // Apple recommends to re-assign "self" with the "super" return value
    if (BugsTestBaseLayer::init())
    {
        setTouchEnabled(true);
        // ask director the the window size
        Size size = Director::getInstance()->getWinSize();
        LayerColor *layer;
        for( int i=0;i < 5;i++)
        {
            layer = LayerColor::create(Color4B(i*20, i*20, i*20,255));
            layer->setContentSize(Size(i*100, i*100));
            layer->setPosition(Point(size.width/2, size.height/2));
            layer->setAnchorPoint(Point(0.5f, 0.5f));
            layer->ignoreAnchorPointForPosition(false);
            addChild(layer, -1-i);
        }

        // create and initialize a Label
        LabelTTF *label = LabelTTF::create("Hello World", "Marker Felt", 64);
        MenuItem *item1 = MenuItemFont::create("restart", CC_CALLBACK_1(Bug914Layer::restart, this));

        Menu *menu = Menu::create(item1, NULL);
        menu->alignItemsVertically();
        menu->setPosition(Point(size.width/2, 100));
        addChild(menu);

        // position the label on the center of the screen
        label->setPosition(Point( size.width /2 , size.height/2 ));

        // add the label as a child to this Layer
        addChild(label);
        return true;
    }
    return false;
}

void Bug914Layer::ccTouchesMoved(Set *touches, Event * event)
{
    log("Number of touches: %d", touches->count());
}

void Bug914Layer::ccTouchesBegan(Set *touches, Event * event)
{
    ccTouchesMoved(touches, event);
}

void Bug914Layer::restart(Object* sender)
{
    Director::getInstance()->replaceScene(Bug914Layer::scene());
}
