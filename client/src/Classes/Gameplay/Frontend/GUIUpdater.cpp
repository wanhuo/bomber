#include "GUIUpdater.h"

using namespace Bomber;
using namespace Bomber::Frontend;

//
GUIUpdater *GUIUpdater::getInstance()
{
    static GUIUpdater instance;
    return &instance;
}

//
void GUIUpdater::init( Map* map, Human* player, Layer* layer)
{
    // Init
    _map = map;
    _player = player;
    _layer = layer;
    _resetNow = false;
    _cache = GUICache::getInstance();
    _mediator = Backend::Mediator::getInstance();
    _collisionDetector = new Collisions();

    // Ignore items which will be spawned, backend will take care of them
    _map->getTiledMap()->layerNamed("sprites2spawn")->setVisible(false);
    _map->getTiledMap()->layerNamed("effects2spawn")->setVisible(false);

    // Hide sprites, obstacles, effects
    _map->getTiledMap()->layerNamed("sprites")->setVisible(false);
    _map->getTiledMap()->layerNamed("obstacles")->setVisible(false);
    _map->getTiledMap()->layerNamed("effects")->setVisible(false);

    // Init Batch Node
    _batchNode = SpriteBatchNode::create("tiles/tileset.png");
    _batchNode->setTag(0);

    // Add BatchNode
    _map->addChild(_batchNode);

    // Initialize player
    this->initPlayer();

    // Cache data from tiled map layers
    _cache->setBatchNode(_batchNode);   // Batch Node init must be first
    _cache->cacheAllLayers(_map);
    _cache->initCaches(_map);

    // Initialize all important layers
    this->initLayers();

    // Init Collision Detector
    _collisionDetector->setMapDimensions(_map->getWidth(), _map->getHeight());
    _collisionDetector->setObstacles(&_obstacles);
    _collisionDetector->setBombs(&_bombs);
}

//
void GUIUpdater::update(Point playerPosition)
{
    Backend::GameState* state = _mediator->getState();
    auto changes = state->getChangesFromId(_lastChangeID);

    _lastChangeID = changes.first;
    for(auto change : changes.second)
    {
        Backend::GameStateChange *GSChange = change;

        switch(GSChange->getType())
        {
            // Move
            case Backend::SPRITE_MOVE:
            {
                this->updateSpriteMove( (Backend::GSCSpriteMove *)GSChange );
            }
            break;
            // Portals
            case Backend::SPRITE_TELEPORT:
            {
                this->updateSpriteTeleport( (Backend::GSCSpriteTeleport *)GSChange, playerPosition );
            }
            break;
            // Bomb spawn
            case Backend::BOMB_SPAWN:
            {
                this->updateBombSpawn( (Backend::GSCBombSpawn *)GSChange );
            }
            break;
            // Bomb destroy
            case Backend::BOMB_DESTROY:
            {
                this->updateBombDestroy( (Backend::GSCBombDestroy *)GSChange );
            }
            break;
            // Obstacle destroy
            case Backend::OBSTACLE_DESTROY:
            {
                this->updateObstacleDestroy( (Backend::GSCObstacleDestroy *)GSChange );
                
            }
            break;
            // Explosion
            case Backend::EXPLOSION_SPAWN:
            {
                this->updateExplosionSpawn( (Backend::GSCExplosionSpawn *)GSChange );
                
            }
            break;
            case Backend::ACHIEVEMENT_UNLOCKED:
            {
                this->updateAchievementUnlocked( (Backend::GSCAchievementUnlocked *)GSChange );
            }
            break;
            case Backend::OBSTACLE_SPAWN:
            {
                this->updateObstacleSpawn( (Backend::GSCObstacleSpawn *)GSChange );
            }
            break;
            case Backend::SPRITE_DESTROY:
            {
                this->updateSpriteDestroy( (Backend::GSCSpriteDestroy *)GSChange );
            }
            break;
            case Backend::LEVER_SWITCH_ON:
            {
                this->updateLeverSwitchOn( (Backend::GSCLeverSwitchOn *)GSChange );
            }
            break;
            case Backend::LEVER_SWITCH_OFF:
            {
                this->updateLeverSwitchOff( (Backend::GSCLeverSwitchOff *)GSChange );
            }
            break;
            case Backend::SPRITE_DAMAGE:
            {
                this->updateSpriteDamage( (Backend::GSCSpriteDamage *)GSChange );
            }
            break;
            case Backend::SPRITE_ATTR_UPDATE:
            {
                this->updateSpriteAttrUpdate( (Backend::GSCSpriteAttrUpdate *)GSChange );
            }
            break;
            case Backend::EFFECT_DESTROY:
            {
                this->updateEffectDestroy( (Backend::GSCEffectDestroy *)GSChange );
            }
            break;
            case Backend::SPRITE_SPAWN:
            {
                this->updateSpriteSpawn( (Backend::GSCSpriteSpawn *)GSChange );
            }
            break;
            case Backend::EFFECT_SPAWN:
            {
                this->updateEffectSpawn( (Backend::GSCEffectSpawn *)GSChange );
            }
            break;
            case Backend::LEVEL_RESET:
            {
                this->updateLevelReset( (Backend::GSCLevelReset *)GSChange );
            }
            break;
            case Backend::LEVEL_FINISH:
            {
                this->updateLevelFinish( (Backend::GSCLevelFinish *)GSChange );
            }
            break;
            case Backend::BOMB_MOVE:
            {
                this->updateBombMove( (Backend::GSCBombMove *)GSChange );
            }
            break;
            // Nothing    
            default: {}
        }
    }

    return;
}

//
void GUIUpdater::updateSpriteMove(Backend::GSCSpriteMove *spriteMove)
{
    if(spriteMove->getGameObjectId() == _player->getID())
    {
        // Only set Z coordinate
        _batchNode->reorderChild(
            _player, 
            _map->getHeight()*TILE_HEIGHT - _player->getPosition().y
        );
        return;
    }

    // Sprite is already initialized
    if( _mobs.find(spriteMove->getGameObjectId()) != _mobs.end() )
    {
        _mobs[spriteMove->getGameObjectId()]->setPosition(
                    ccp (
                        spriteMove->getPosition().x,
                        spriteMove->getPosition().y
                    )
                );
        _batchNode->reorderChild(
            _mobs[spriteMove->getGameObjectId()],
            _map->getHeight()*TILE_HEIGHT - spriteMove->getPosition().y - TILE_HEIGHT/4 // DO NOT CHANGE
        );
    }
    // First occurence of a sprite
    else
    {
        std::cerr << "Unitialized sprite: " << spriteMove->getGameObjectId() << std::endl;
    }

    return;
}


//
void GUIUpdater::updateSpriteTeleport(Backend::GSCSpriteTeleport *spriteTeleport, Point playerPosition)
{
    // Player, map needs to be moved
    if(spriteTeleport->getGameObjectId() == _player->getID())
    {
        Point teleportPosition = ccp( spriteTeleport->getPosition().x, spriteTeleport->getPosition().y);
        _player->setPosition(teleportPosition);
        _map->addToPosition(ccpSub(playerPosition, teleportPosition));
    }
    // Mob
    else
    {
        _mobs[spriteTeleport->getGameObjectId()]->setPosition(
                    ccp (
                        spriteTeleport->getPosition().x,
                        spriteTeleport->getPosition().y
                    )
                );
    }
    return;
}

//
void GUIUpdater::updateBombSpawn(Backend::GSCBombSpawn *bombSpawn)
{

    unsigned int id = bombSpawn->getGameObjectId();
    Point bombSpawnPosition = ccp( bombSpawn->getPosition().x, bombSpawn->getPosition().y);

    _bombs[ id ] = _cache->getBomb(
        _batchNode->getTexture(),
        this->pickImageFromTexture(BOMB_IMAGE_ID)
    );

    _bombs[ id ]->setPosition(bombSpawnPosition);
    _batchNode->reorderChild(_bombs[ id ], _map->getHeight()*TILE_HEIGHT - bombSpawnPosition.y);

    // for kicking
    _collisionDetector->setCFA(id, bombSpawnPosition);

    return;
}

//
void GUIUpdater::updateBombDestroy(Backend::GSCBombDestroy *bombDestroy)
{
    unsigned int id = bombDestroy->getGameObjectId();
    // Cache or remove         
    if ( !_cache->cacheBomb(_bombs[ id ]) )
    {
        _batchNode->removeChild(_bombs[ id ], true);
    }
    _bombs.erase(id);
}

//
void GUIUpdater::updateObstacleDestroy(Backend::GSCObstacleDestroy *obstacleDestroy)
{
    int bombID = (_map->getHeight() - ( obstacleDestroy->getGameObjectId() / _map->getWidth() ) - 1) 
                * _map->getWidth()
                + obstacleDestroy->getGameObjectId() % _map->getWidth();
    // Cache or remove         
    if ( !_cache->cacheObstacle(_obstacles[bombID]) )
    {
        _batchNode->removeChild(_obstacles[bombID], true);
    }
    _obstacles.erase(bombID); // Collision detection

}

//
void GUIUpdater::updateExplosionSpawn(Backend::GSCExplosionSpawn *explosionSpawn)
{
    Point epicentrum = ccp(explosionSpawn->getEpicentrum().x, explosionSpawn->getEpicentrum().y);
    Explosion *explosion = new Explosion(
        epicentrum,
        explosionSpawn->getLeftArmLength(),
        explosionSpawn->getRightArmLength(),
        explosionSpawn->getTopArmLength(),
        explosionSpawn->getBottomArmLength()
    );
    explosion->autorelease();
    _map->addChild(explosion, 0);
    return;
}

//
void GUIUpdater::updateAchievementUnlocked(Backend::GSCAchievementUnlocked *achievementUnlocked)
{

    AchievementButton *ab = new AchievementButton(
        achievementUnlocked->getTitle(),
        achievementUnlocked->getDescription(),
        ""
    );
    ButtonLayer::getInstance()->addToAchievements(ab);
    return;
}

//
void GUIUpdater::updateObstacleSpawn(Backend::GSCObstacleSpawn *obstacleSpawn)
{
    unsigned int ix = obstacleSpawn->getCoordinates().x;
    unsigned int iy = obstacleSpawn->getCoordinates().y;
    unsigned int transformed_iy = _map->getHeight() - iy - 1;
    int position = _map->getWidth() * transformed_iy + ix;

    // Init with texture of Batch Node
    _obstacles[ position ] = _cache->getObstacle(
        _batchNode->getTexture(),
        this->pickImageFromTexture( obstacleSpawn->getGid() )
    );

    // Add to Batch Node
    _obstacles[ position ]->setPosition( ccp(ix*TILE_WIDTH, iy*TILE_HEIGHT ) );
    _batchNode->reorderChild(_obstacles[ position ], transformed_iy*TILE_HEIGHT+5);

    return;
}

//
void GUIUpdater::updateSpriteDestroy( Backend::GSCSpriteDestroy *spriteDestroy )
{
    if(spriteDestroy->getGameObjectId() == _player->getID())
    {
        _batchNode->removeChild(_player, true); // WARNING
        _playerDestroyed = true;
        _collisionDetector->skipEval(true);
        return;
    }
    unsigned int id = spriteDestroy->getGameObjectId();
    // Cache or remove         
    if ( !_cache->cacheSprite(_mobs[id]) )
    {
        _batchNode->removeChild(_mobs[id], true);
    }
    _mobs.erase(id);
}

//
void GUIUpdater::updateLeverSwitchOn( Backend::GSCLeverSwitchOn *leverSwitchOn )
{
    return; // TODO
}

//
void GUIUpdater::updateLeverSwitchOff( Backend::GSCLeverSwitchOff *leverSwitchOff )
{
    return; // TODO
}

//
void GUIUpdater::updateSpriteDamage( Backend::GSCSpriteDamage *spriteDamage )
{
    return; // TODO
}

//
void GUIUpdater::updateSpriteAttrUpdate( Backend::GSCSpriteAttrUpdate *spriteAttrUpdate )
{
    // Show only players buff
    if( spriteAttrUpdate->getGameObjectId() != _player->getID() )
    {
        return;
    }

    // Get buff image
    unsigned int imageID = 0;
    switch(spriteAttrUpdate->getEffectType())
    {
        case Backend::EFFECT_BOMB_CAPACITY_INC:
            imageID = BOMB_CAPACITY_INC_ETI;
            break;
        case Backend::EFFECT_BOMB_POWER_INC:
            imageID = BOMB_POWER_INC_ETI;
            break;
        case Backend::EFFECT_SPEED_INC:
            imageID = SPEED_INC_ETI;
            _player->setSpeed(_player->getSpeed() + SPRITE_SPEED_INCREASE);
            break;
        default:
            std::cerr << "Unknown effect type: " << 
                spriteAttrUpdate->getEffectType() << std::endl;
            return; // Unknown effect type
    }

    // Create or update buff button
    if( ButtonLayer::getInstance()->isInBuffs(imageID) )
    {
        ButtonLayer::getInstance()->incrementBuff(imageID);
    }
    else
    {
        EffectButton *eb = new EffectButton(
            imageID,
            this->pickImageFromTexture( imageID ),
            _batchNode->getTexture()
        );
        ButtonLayer::getInstance()->addToBuffs(eb);
    }
    return;
}

//
void GUIUpdater::updateEffectDestroy( Backend::GSCEffectDestroy *effectDestroy )
{
    int id = (_map->getHeight() - ( effectDestroy->getGameObjectId() / _map->getWidth() ) - 1) 
                * _map->getWidth()
                + effectDestroy->getGameObjectId() % _map->getWidth();
    // Cache or remove         
    if ( !_cache->cacheEffect(_effects[id]) )
    {
        _batchNode->removeChild(_effects[id], true);
    }
    _effects.erase(id);
    return;
}

//
void GUIUpdater::updateSpriteSpawn( Backend::GSCSpriteSpawn *spriteSpawn )
{
    unsigned int ix = spriteSpawn->getCoordinates().x;
    unsigned int iy = spriteSpawn->getCoordinates().y;
    unsigned int transformed_iy = _map->getHeight() - iy - 1;
    unsigned int id = spriteSpawn->getGameObjectId();

    // Init with texture of Batch Node
    _mobs[ id ] = _cache->getSprite(
        _batchNode->getTexture(),
        this->pickImageFromTexture( spriteSpawn->getGid() ) 
    );

    // Position and Batch node Z order
    _mobs[ id ]->setPosition( ccp(ix*TILE_WIDTH, iy*TILE_HEIGHT ) );
    _batchNode->reorderChild(_mobs[ id ], transformed_iy*TILE_HEIGHT);

    return;
}

//
void GUIUpdater::updateEffectSpawn( Backend::GSCEffectSpawn *effectSpawn )
{
    unsigned int ix = effectSpawn->getCoordinates().x;
    unsigned int iy = effectSpawn->getCoordinates().y;
    unsigned int transformed_iy = _map->getHeight() - iy - 1;
    int position = _map->getWidth() * transformed_iy + ix;

    // Init with texture of Batch Node
    _effects[ position ] = _cache->getEffect(
        _batchNode->getTexture(),
        this->pickImageFromTexture( effectSpawn->getGid() ) 
    );

    // Position and Batch node Z order
    _effects[ position ]->setPosition( ccp(ix*TILE_WIDTH, iy*TILE_HEIGHT ) );
    _batchNode->reorderChild(_effects[ position ], transformed_iy*TILE_HEIGHT+5);

    return;
}

//
void GUIUpdater::updateLevelReset( Backend::GSCLevelReset *levelReset )
{
    _resetNow = true;
    return;
}

//
void GUIUpdater::updateLevelFinish( Backend::GSCLevelFinish *levelFinish )
{
    _resetNow = true; 
    return;
}

//
void GUIUpdater::updateBombMove( Backend::GSCBombMove *bombMove )
{
    unsigned int id = bombMove->getGameObjectId();
    _bombs[ id ]->setPosition( ccp( bombMove->getPosition().x, bombMove->getPosition().y) );
    _batchNode->reorderChild(_bombs[ id ], _map->getHeight()*TILE_HEIGHT - bombMove->getPosition().y);
    return;
}

/*
 * ========== 
 *  General
 * ==========
 */

Rect GUIUpdater::pickImageFromTexture(unsigned int id)
{
    id--;
    int ix = id % TEXTURE_IMAGES_PER_LINE;
    int iy = id / TEXTURE_IMAGES_PER_LINE;

    return CCRectMake(
        TEXTURE_SPACING + (TEXTURE_TILE_WIDTH + TEXTURE_SPACING) * ix,
        TEXTURE_SPACING + (TEXTURE_TILE_HEIGHT + TEXTURE_SPACING) * iy,
        TEXTURE_TILE_WIDTH,
        TEXTURE_TILE_HEIGHT
    );
}

bool GUIUpdater::obstacleExists(unsigned int id)
{
    if( _obstacles[ id ] != NULL )
    {
        return true;
    }
    return false;
}

/*
 * ========== 
 * Collisions
 * ==========
 */
 
//
std::vector<bool> GUIUpdater::evalCollisions(Point currentPoint, Point nextPoint)
{
    return _collisionDetector->eval(currentPoint, nextPoint);
}

/*
 * ========== 
 *    Init
 * ==========
 */

//
void GUIUpdater::initPlayer()
{
    // Eval collisions
    _playerDestroyed = false;

    // All initialization
    _player->initWithTexture(_batchNode->getTexture(), CCRectMake(120,60,80,110));
    _player->retain();
    _player->setAnchorPoint(ccp(0.45f, 0.2f));
    _player->setVertexZ(0);
    
    // Add player to Batch Node
    _batchNode->addChild(_player, 0);
}

//
void GUIUpdater::initLayers()
{
    // Clear
    _mobs.clear();
    _obstacles.clear();
    _effects.clear();
    _bombs.clear();

    // Init obstacles, mobs and effects structure
    for(auto it : *(_cache->getObstacles()) )
    {
        unsigned int id = it.first;
        Sprite *sp = it.second;

        _obstacles[ id ] = Sprite::createWithTexture( sp->getTexture(), sp->getTextureRect() );
        _obstacles[ id ]->setPosition( sp->getPosition() );
        _obstacles[ id ]->setAnchorPoint( ccp(0, 0) );
        _obstacles[ id ]->setVertexZ(0); // DO NOT CHANGE

        _batchNode->addChild(_obstacles[ id ], 0);
        _batchNode->reorderChild(
            _obstacles[ id ],
            _map->getHeight()*TILE_HEIGHT - sp->getPosition().y + 5
        );
    }
    for(auto it : *(_cache->getMobs()) )
    {
        unsigned int id = it.first;
        Sprite *sp = it.second;

        _mobs[ id ] = Sprite::createWithTexture( sp->getTexture(), sp->getTextureRect() );
        _mobs[ id ]->setPosition( sp->getPosition() );
        _mobs[ id ]->setAnchorPoint( ccp(0, 0) );
        _mobs[ id ]->setVertexZ(0); // DO NOT CHANGE

        _batchNode->addChild(_mobs[ id ], 0);
        _batchNode->reorderChild(
            _mobs[ id ],
            _map->getHeight()*TILE_HEIGHT - sp->getPosition().y - TILE_HEIGHT/4
        );
    }
    for(auto it : *(_cache->getEffects()) )
    {
        unsigned int id = it.first;
        Sprite *sp = it.second;

        _effects[ id ] = Sprite::createWithTexture( sp->getTexture(), sp->getTextureRect() );
        _effects[ id ]->setPosition( sp->getPosition() );
        _effects[ id ]->setAnchorPoint( ccp(0, 0) );
        _effects[ id ]->setVertexZ(0); // DO NOT CHANGE

        _batchNode->addChild(_effects[ id ], 0);
        _batchNode->reorderChild(
            _effects[ id ],
            _map->getHeight()*TILE_HEIGHT - sp->getPosition().y + 5
        );
    }

    return;
}

/*
 * ========== 
 *   Reset
 * ==========
 */

//
void GUIUpdater::resetGUI()
{
    // Buffs and Achievements
    ButtonLayer::getInstance()->reset();

    // Batch Node
    _batchNode->removeAllChildrenWithCleanup(true);
    this->initLayers();

    // Cache
    _cache->resetSprites();
    _cache->initCaches(_map);

    // Player
    this->initPlayer();

    // Collisions
    _collisionDetector->reset();

    // Init instance variable
    _resetNow = false;

    return;
}
