#include "Bubble.h"

using namespace Bomber::Frontend;

Bubble::Bubble(std::string title, std::string description, std::string image)
{
    Node *bubble = new Node();
    int fontSize = 30;

    Size visibleSize = Director::sharedDirector()->getVisibleSize();

    // Icon
    Sprite *icon = new Sprite();
    icon->initWithFile("tiles/images/test_50.png"); // Replace with image
    icon->setAnchorPoint(ccp(0,0));
    Rect bBox = icon->boundingBox();
    icon->setPosition( ccp(
            0,
            visibleSize.height/5 + visibleSize.height/4.3     
        )
    );
    bubble->addChild(icon, 10);

    // Text
    LabelTTF* titleText = LabelTTF::create(
        title.c_str(),
        TEXT_FONT,
        fontSize,
        CCSizeMake(visibleSize.width, fontSize), 
        kTextAlignmentCenter
    );
    LabelTTF* descriptionText = LabelTTF::create(
        description.c_str(),
        TEXT_FONT,
        fontSize * 0.6,
        CCSizeMake(visibleSize.width, fontSize), 
        kTextAlignmentCenter
    );

    // Border
    auto dr = DrawNode::create();
    float borderSize = 3.5;
    int bBoxWidth = bBox.getMaxX() - bBox.getMinX();
    Point *ccPoints = Shapes::getBubbleBox(
        visibleSize.width,
        visibleSize.height,
        bBoxWidth
    );
    dr->DrawNode::drawPolygon(
        ccPoints,
        7,
        ccc4f(0.0, 0.0, 0.0, 0.8),
        borderSize,
        ccc4f(0.8, 0.6, 0.4, 0.7)
    );
    bubble->addChild(dr); 

    // Positions
    titleText->setPosition( 
        ccp(
            ccPoints[6].x + 8*fontSize,
            ccPoints[6].y - 1*fontSize
        )
    );
    descriptionText->setPosition( 
        ccp(
            ccPoints[6].x + 10*fontSize,
            ccPoints[6].y - 3*fontSize
        )
    );
    bubble->addChild(titleText);
    bubble->addChild(descriptionText); 
      
    _button = bubble;
}