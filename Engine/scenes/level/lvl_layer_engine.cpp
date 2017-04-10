/*
 * Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2017 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lvl_layer_engine.h"
#include <data_configs/config_manager.h>
#include "../scene_level.h"

LVL_LayerEngine::LVL_LayerEngine(LevelScene *_parent) :
    m_scene(_parent),
    m_layers()
{
    // Automatically initialize special layers
    {
        Layer &db = m_layers[DESTROYED_LAYER_NAME];
        db.m_layerType = Layer::T_DESTROYED_BLOCKS;
        db.m_vizible = false;
        db.m_rtree.m_scene = m_scene;
    }
    {
        Layer &db = m_layers[SPAWNED_LAYER_NAME];
        db.m_layerType = Layer::T_SPAWNED_NPCs;
        db.m_vizible = true;
        db.m_rtree.m_scene = m_scene;
    }
}

void LVL_LayerEngine::spawnSmokeAt(double x, double y)
{
    SpawnEffectDef smoke = ConfigManager::g_setup_effects.m_smoke;
    smoke.startX = x;
    smoke.startY = y;
    m_scene->launchEffect(smoke, true);
}

void LVL_LayerEngine::hide(std::string layer, bool smoke)
{
    Layer &lyr = m_layers[layer];

    for(Layer::Members::iterator it = lyr.m_members.begin();
        it != lyr.m_members.end();
        it++)
    {
        PGE_Phys_Object *body = it->second;
        if(!body->isVisible())
            continue;
        body->hide();
        if(smoke)
            spawnSmokeAt(body->posCenterX(), body->posCenterY());
    }

    if(lyr.m_layerType == Layer::T_REGULAR)
        lyr.m_vizible = false;
}

void LVL_LayerEngine::show(std::string layer, bool smoke)
{
    Layer &lyr = m_layers[layer];

    if(lyr.m_layerType == Layer::T_DESTROYED_BLOCKS)
    {
        //Restore all destroyed and modified blocks into their initial states
        for(Layer::Members::iterator it = lyr.m_members.begin();
            it != lyr.m_members.end();
            it++)
        {
            LVL_Block *blk = dynamic_cast<LVL_Block *>(it->second);
            if(!blk)
                continue;
            //lyr.m_members.remove(intptr_t(blk));
            bool wasInviz = !blk->isVisible();
            blk->setVisible(true);
            blk->init(true);//Force re-initialize block from initial data
            if(smoke && wasInviz)
                spawnSmokeAt(blk->posCenterX(), blk->posCenterY());
        }

        //Clear layer from objects
        lyr.m_members.clear();
    }
    else
    {
        for(Layer::Members::iterator it = lyr.m_members.begin();
            it != lyr.m_members.end();
            it++)
        {
            PGE_Phys_Object *body = it->second;
            if(body->isVisible())
                continue;
            body->show();
            if(smoke)
                spawnSmokeAt(body->posCenterX(), body->posCenterY());
        }
    }

    if(lyr.m_layerType == Layer::T_REGULAR)
        lyr.m_vizible = true;
}

void LVL_LayerEngine::toggle(std::string layer, bool smoke)
{
    Layer &lyr = m_layers[layer];
    bool viz = !lyr.m_vizible;

    if(viz && (lyr.m_layerType == Layer::T_DESTROYED_BLOCKS))
    {
        show(layer);
        return;
    }
    else if(!viz && (lyr.m_layerType == Layer::T_SPAWNED_NPCs))
    {
        hide(layer);
        return;
    }

    for(Layer::Members::iterator it = lyr.m_members.begin(); it != lyr.m_members.end(); it++)
    {
        PGE_Phys_Object *body = it->second;
        body->setVisible(viz);
        if(smoke)
            spawnSmokeAt(body->posCenterX(), body->posCenterY());
    }

    if(lyr.m_layerType == Layer::T_REGULAR)
        lyr.m_vizible = viz;
}

void LVL_LayerEngine::registerItem(std::string layer, PGE_Phys_Object *item)
{
    //Register item in the layer
    Layer &lyr = m_layers[layer];
    if( (item->type == PGE_Phys_Object::LVLBGO)||
        (item->type == PGE_Phys_Object::LVLBlock)||
        (item->type == PGE_Phys_Object::LVLWarp)||
        (item->type == PGE_Phys_Object::LVLPhysEnv) )
    {
        if(!lyr.m_rtree.m_scene)
        {
            lyr.m_rtree.m_scene = m_scene;
            lyr.m_rtree.setSize(1.0, 1.0);
            lyr.m_rtree.m_momentum = item->m_momentum;
            //Keep initial position of layer itself
            lyr.m_rtree.m_momentum_relative = lyr.m_rtree.m_momentum;
            item->m_parent = &lyr.m_rtree;
            m_scene->registerElement(&lyr.m_rtree);
        } else {
            if(lyr.m_rtree.left() > item->left())
                lyr.m_rtree.setLeft(item->left());
            if(lyr.m_rtree.top() > item->top())
                lyr.m_rtree.setTop(item->top());
            if(lyr.m_rtree.right() < item->right())
                lyr.m_rtree.setRight(item->right());
            if(lyr.m_rtree.bottom() < item->bottom())
                lyr.m_rtree.setBottom(item->bottom());
            //Keep initial position of layer itself
            lyr.m_rtree.m_momentum_relative = lyr.m_rtree.m_momentum;
            lyr.m_rtree.m_momentum_relative.x -= lyr.m_rtree.m_offsetX;
            lyr.m_rtree.m_momentum_relative.y -= lyr.m_rtree.m_offsetY;
            lyr.m_rtree.m_momentum_relative.saveOld();
            item->m_parent = &lyr.m_rtree;
            //item->_syncPosition();
        }
        item->m_momentum_relative   = item->m_momentum;
        item->m_momentum_relative.x += lyr.m_rtree.m_offsetX;
        item->m_momentum_relative.y += lyr.m_rtree.m_offsetY;
        item->m_momentum_relative.saveOld();
        item->_syncPosition();
        m_scene->unregisterElement(item);
    }

    lyr.m_members[reinterpret_cast<intptr_t>(item)] = item;
    if(lyr.m_layerType == Layer::T_REGULAR)
        item->setVisible(lyr.m_vizible);
}

void LVL_LayerEngine::removeRegItem(std::string layer, PGE_Phys_Object *item)
{
    //Remove item from the layer
    Layer &lyr = m_layers[layer];
    if( (item->type == PGE_Phys_Object::LVLBGO)||
        (item->type == PGE_Phys_Object::LVLBlock)||
        (item->type == PGE_Phys_Object::LVLWarp)||
        (item->type == PGE_Phys_Object::LVLPhysEnv) )
    {
        if(lyr.m_rtree.m_scene)
        {
            item->m_momentum_relative.x -= lyr.m_rtree.m_offsetX;
            item->m_momentum_relative.y -= lyr.m_rtree.m_offsetY;
            item->m_parent = nullptr;
            lyr.m_rtree.unregisterElement(item);
        }
    }
    lyr.m_members.erase(reinterpret_cast<intptr_t>(item));
}

void LVL_LayerEngine::installLayerMotion(std::string layer, double speedX, double speedY)
{
    MovingLayersTable::iterator mv = m_movingLayers.find(layer);
    if(mv != m_movingLayers.end())
    {
        MovingLayer &l = mv->second;
        l.m_speedX = speedX;
        l.m_speedY = speedY;
    }
    else
    {
        if((speedX == 0.0) && (speedY == 0.0))
            return;//Don't store zero-speed layers!
        LayersTable::iterator li = m_layers.find(layer);
        if(li == m_layers.end())
            return;
        Layer &lyr = li->second;
        MovingLayer l;
        l.m_speedX = speedX;
        l.m_speedY = speedY;
        l.m_members = &lyr.m_members;
        l.m_subtree = &lyr.m_rtree;
        m_movingLayers.insert({layer, l});
    }
}

void LVL_LayerEngine::processMoving(double tickTime)
{
    if(m_movingLayers.empty())
        return;

    std::vector<std::string> remove_list;
    for(MovingLayersTable::iterator layerIt = m_movingLayers.begin();
        layerIt != m_movingLayers.end();
        layerIt++)
    {
        MovingLayer &l = layerIt->second;
        l.m_subtree->m_momentum.velX    = l.m_speedX;
        l.m_subtree->m_momentum.velXsrc = l.m_speedX;
        l.m_subtree->m_momentum.velY    = l.m_speedY;
        l.m_subtree->iterateStep(tickTime);
        l.m_subtree->m_offsetX    = l.m_subtree->m_momentum_relative.x - l.m_subtree->m_momentum.x;
        l.m_subtree->m_offsetY    = l.m_subtree->m_momentum_relative.y - l.m_subtree->m_momentum.y;
        l.m_subtree->m_offsetXold = l.m_subtree->m_momentum_relative.x - l.m_subtree->m_momentum.oldx;
        l.m_subtree->m_offsetYold = l.m_subtree->m_momentum_relative.y - l.m_subtree->m_momentum.oldy;
        l.m_subtree->_syncPosition();
//        for(Layer::Members::iterator it = l.m_members->begin(); it != l.m_members->end(); it++)
//        {
//            PGE_Phys_Object *obj = it->second;
//            //Don't iterate playable characters
//            if(obj->type == PGE_Phys_Object::LVLPlayer)
//                continue;

//            //obj->setSpeed(l.m_speedX, l.m_speedY);

//            ////Don't iterate activated NPC's!
//            //if(obj->type == PGE_Phys_Object::LVLNPC)
//            //{
//            //    LVL_Npc *npc = dynamic_cast<LVL_Npc *>(obj);
//            //    SDL_assert_release(npc);
//            //    if(npc)
//            //    {
//            //        if(npc->isActivated /* &&
//            //           !npc->isGenerator &&
//            //           !npc->is_scenery*/)
//            //            continue;
//            //    }
//            //    else
//            //        continue;
//            //}
//            ////obj->iterateStep(tickTime, true);
//            //if((l.m_speedX == 0.0) && (l.m_speedY == 0.0))
//            //{
//            //    if(obj->m_bodytype == PGE_physBody::Body_STATIC)
//            //    {
//            //        //obj->m_momentum.y = obj->m_momentum.y < 0 ? floor(obj->m_momentum.y) : ceil(obj->m_momentum.y);
//            //        obj->m_momentum.saveOld();
//            //    }
//            //}
//            //obj->_syncPosition();
//        }
        if((l.m_speedX == 0.0) && (l.m_speedY == 0.0))
            remove_list.push_back(layerIt->first);
    }
    //Remove zero-speed layers
    for(size_t i = 0; i < remove_list.size(); i++)
        m_movingLayers.erase(remove_list[i]);
}

bool LVL_LayerEngine::isEmpty(std::string layer)
{
    LayersTable::iterator i = m_layers.find(layer);
    return (i == m_layers.end()) || i->second.m_members.empty();
}

void LVL_LayerEngine::clear()
{
    m_movingLayers.clear();
    m_layers.clear();
}

