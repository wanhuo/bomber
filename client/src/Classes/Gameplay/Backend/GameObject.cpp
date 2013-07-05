
#include <stdio.h>

#include "GameObject.h"
#include "../../Constants.h"

using namespace Bomber::Backend;

void GameObject::setPosition(int x, int y)
{
    _position = Position(x, y);
    _dirty = true;
}

void GameObject::setPosition(Position position)
{
    _position = position;
    _dirty = true;
}

void GameObject::setSize(int width, int height)
{
    _size = Size(width, height);
}

void GameObject::setSize(Size size)
{
    _size = size;
}

Coordinates GameObject::getCoords()
{
    Position center = this->getCollisionRect().getCenterPosition();

    return Coordinates(
            center.x/TILE_WIDTH,
            center.y/TILE_HEIGHT
    );
}

bool GameObject::collides(GameObject *object)
{
    if (!_circleCollider && !object->hasCircleCollider())
    {
        return this->getCollisionRect().isIntersecting(object->getCollisionRect());
    }
    else
    {
        printf("TODO implement circle X rect, circle X circle - %s X %s\n", _name, object->getName());
    }
    return false;
}

