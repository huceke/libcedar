/*
 *      Copyright (C) 2012 Edgar Hucek
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if defined(HAVE_LIBCEDAR)

#define CLASSNAME "CCedarFrameBufferManager"

#include "CedarFrameBufferManager.h"
#include "CedarVEHwControll.h"
#include "CedarDecoder.h"
#include "libvecore/libve.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#define MAX_FRAMES 64

CCedarFrameBufferManager::CCedarFrameBufferManager()
{
  pthread_mutex_init(&m_lock, NULL);
  pthread_mutex_init(&m_lockDisplay, NULL);

  m_open = false;
  m_frames = 0;
  m_readyFrames = 0;
  m_freeFrames = 0;
}

CCedarFrameBufferManager::~CCedarFrameBufferManager()
{
  //FbmRelease(this, NULL);
  pthread_mutex_destroy(&m_lock);
  pthread_mutex_destroy(&m_lockDisplay);
}

FbmPicture_t *CCedarFrameBufferManager::GetFbmPicture(vpicture_t *picture)
{
  FbmPicture_t *fbmPicture = NULL;

  for(unsigned int i = 0; i < m_buffers.size(); i++)
  {
    FbmPicture_t *fbmPictureInternal = m_buffers[i];
    if(&fbmPictureInternal->picture == picture)
    {
      fbmPicture = fbmPictureInternal;
      break;
    }
  }

  return fbmPicture;
}

vpicture_t* CCedarFrameBufferManager::FbmGetDisplayFrame()
{
  vpicture_t *picture = NULL;

  LockDisplay();

  if(!m_display.empty())
  {
    picture = m_display.front();
    m_display.pop_front();
  }

  //if(picture)
  //  fprintf(stderr, "%s::%s picture %d %dx%d\n", CLASSNAME, __func__, picture->id, picture->width, picture->height);

  m_readyFrames = m_display.size();
  UnLockDisplay();

  return picture;
}

vpicture_t* CCedarFrameBufferManager::FbmGetDisplayFrameStatic(Handle h)
{
  CCedarFrameBufferManager *manager = (CCedarFrameBufferManager *)h;

  if(manager)
    return manager->FbmGetDisplayFrame();
  else
    return NULL;
}

void CCedarFrameBufferManager::SetFbmPictureState(vpicture_t *picture, enum PictureState pictureState)
{
  FbmPicture_t *fbmPicture = GetFbmPicture(picture);

  if(fbmPicture)
  {
    fbmPicture->pictureState = pictureState;
  }

  assert(fbmPicture != NULL);
}

enum PictureState CCedarFrameBufferManager::GetFbmPictureState(vpicture_t *picture)
{
  FbmPicture_t *fbmPicture = GetFbmPicture(picture);

  PictureState pictureState = STATE_UNDEFINED;

  if(fbmPicture)
  {
    pictureState = fbmPicture->pictureState;
  }

  //assert(pictureState != STATE_UNDEFINED);

  return pictureState;
}

void CCedarFrameBufferManager::FbmRelease(Handle h, void* parent)
{
  LockDisplay();
  m_display.clear();
  m_readyFrames = m_display.size();
  UnLockDisplay();

  Lock();

  for(unsigned int i = 0; i < m_buffers.size(); i++)
  {
    FbmPicture_t *fbmPicture = m_buffers[i];
    vpicture_t *picture = &fbmPicture->picture;

    if(picture->y)
      CCedarVEHwControll::MemPfreeStatic(picture->y);

    if(picture->u)
      CCedarVEHwControll::MemPfreeStatic(picture->u);

    if(picture->v)
      CCedarVEHwControll::MemPfreeStatic(picture->v);

    if(picture->alpha)
      CCedarVEHwControll::MemPfreeStatic(picture->alpha);

    free(fbmPicture);
  }

  m_buffers.clear();

  m_available.clear();
  m_freeFrames = m_available.size();

  UnLock();
}

void CCedarFrameBufferManager::FbmReleaseStatic(Handle h, void *parent)
{
  CCedarFrameBufferManager *manager = (CCedarFrameBufferManager *)h;
  if(manager)
  {
    manager->FbmRelease(h, parent);
    delete manager;
  }
}

vpicture_t* CCedarFrameBufferManager::FbmRequestFrame(Handle h)
{
  vpicture_t *picture = NULL;

  Lock();

  if(!m_available.empty())
  {
    picture = m_available.front();
    m_available.pop_front();
  }

  if(picture)
  {
    //printf("CCedarFrameBufferManager::FbmRequestFrame %d\n", picture->id);
    SetFbmPictureState(picture, STATE_DECODER);
  }

  m_freeFrames = m_available.size();
  UnLock();

  return picture;
}

vpicture_t* CCedarFrameBufferManager::FbmRequestFrameStatic(Handle h)
{
  CCedarFrameBufferManager *manager = (CCedarFrameBufferManager *)h;

  if(manager)
    return manager->FbmRequestFrame(h);
  else
    return NULL;
}

void CCedarFrameBufferManager::FbmReturnDisplayFrame(vpicture_t* picture, Handle h)
{
  Lock();

  if(picture)
  {
    PictureState pictureState = GetFbmPictureState(picture);
    if(pictureState == STATE_DISPLAY)
    {
      SetFbmPictureState(picture, STATE_FREE);
      m_available.push_back(picture);
    }
    else if(pictureState == STATE_SHARED)
    {
      SetFbmPictureState(picture, STATE_SHARED_FREE);
    }

    //fprintf(stderr, "%s::%s picture %d %dx%d\n", CLASSNAME, __func__, picture->id, picture->width, picture->height);
  }

  m_freeFrames = m_available.size();
  UnLock();
}

void CCedarFrameBufferManager::FbmReturnFrame(vpicture_t* picture, u8 valid, Handle h)
{
  if(!picture)
  {
    return;
  }

  //fprintf(stderr, "%s::%s picture %d %dx%d valid %d\n", CLASSNAME, __func__, picture->id, picture->width, picture->height, valid);

  PictureState pictureState = GetFbmPictureState(picture);
  if(pictureState == STATE_DECODER)
  {
    if(valid)
    {
      LockDisplay();
      SetFbmPictureState(picture, STATE_DISPLAY);
      m_display.push_back(picture);
      m_readyFrames = m_display.size();
      UnLockDisplay();
      //printf("CCedarFrameBufferManager::FbmReturnFrame %d valid %d state %d\n", picture->id, valid, GetFbmPictureState(picture));
    }
    else
    {
      Lock();
      SetFbmPictureState(picture, STATE_FREE);
      m_available.push_back(picture);
      m_freeFrames = m_available.size();
      UnLock();
    }
  }
  else if(pictureState == STATE_SHARED)
  {
    Lock();
    SetFbmPictureState(picture, STATE_DISPLAY);
    UnLock();
  }
  else if(pictureState == STATE_SHARED_FREE)
  {
    Lock();
    SetFbmPictureState(picture, STATE_FREE);
    m_available.push_back(picture);
    m_freeFrames = m_available.size();
    UnLock();
  }
  else if(pictureState == STATE_UNDEFINED)
  {
    if(picture)
    {
      Lock();
      SetFbmPictureState(picture, STATE_FREE);
      m_available.push_back(picture);
      m_freeFrames = m_available.size();
      UnLock();
    }
  }

}

void CCedarFrameBufferManager::FbmReturnFrameStatic(vpicture_t* picture, u8 valid, Handle h)
{
  CCedarFrameBufferManager *manager = (CCedarFrameBufferManager *)h;

  if(manager)
    manager->FbmReturnFrame(picture, valid, h);
}

void CCedarFrameBufferManager::FbmShareFrame(vpicture_t* picture, Handle h)
{
  LockDisplay();

  SetFbmPictureState(picture, STATE_SHARED);

  m_display.push_back(picture);
  m_readyFrames = m_display.size();

  //if(picture)
  //  fprintf(stderr, "%s::%s picture->id %d\n", CLASSNAME, __func__, picture->id);

  UnLockDisplay();
}

void CCedarFrameBufferManager::FbmShareFrameStatic(vpicture_t* picture, Handle h)
{
  CCedarFrameBufferManager *manager = (CCedarFrameBufferManager *)h;

  if(manager)
    manager->FbmShareFrame(picture, h);
}

Handle CCedarFrameBufferManager::FbmInit(u32 max_frame_num,
                u32 min_frame_num, u32 size_y, u32 size_u,
                u32 size_v, u32 size_alpha, pixel_format_e format)
{
  Lock();
  
  m_frames = std::max(min_frame_num, max_frame_num) + 10;

  if(m_frames > MAX_FRAMES)
    return NULL;

  for(int i = 0; i < m_frames; i++)
  {
    FbmPicture_t *fbmPicture = (FbmPicture_t *)malloc(sizeof(FbmPicture_t));
    memset(fbmPicture, 0x0, sizeof(FbmPicture_t));
    vpicture_t *picture = &fbmPicture->picture;

    picture->id          = i;
    picture->size_y      = size_y;
    picture->size_u      = size_u;
    picture->size_v      = size_v;
    picture->size_alpha  = size_alpha;

    if(picture->size_y)
    {
      picture->y = (u8*)CCedarVEHwControll::MemPallocStatic(picture->size_y, 1024);
      if(!picture->y)
        return NULL;
    }
    if(picture->size_u)
    {
      picture->u = (u8*)CCedarVEHwControll::MemPallocStatic(picture->size_u, 1024);
      if(!picture->u)
        return NULL;
    }
    if(picture->size_v)
    {
      picture->v = (u8*)CCedarVEHwControll::MemPallocStatic(picture->size_v, 1024);
      if(!picture->v)
        return NULL;
    }
    if(picture->size_alpha)
    {
      picture->alpha = (u8*)CCedarVEHwControll::MemPallocStatic(picture->size_alpha, 1024);
      if(!picture->alpha)
        return NULL;
    }

    fbmPicture->pictureState = STATE_FREE;

    m_buffers.push_back(fbmPicture);
    m_available.push_back(picture);
  }

  //fprintf(stderr, "%s::%s min_frame_num %d max_frame_num %d frames %d\n", CLASSNAME, __func__, 
  //    min_frame_num, max_frame_num, frames);
  m_open = true;

  m_readyFrames = 0;
  m_freeFrames = m_available.size();
  m_readyFrames = m_display.size();

  UnLock();
  return this;
}

Handle CCedarFrameBufferManager::FbmInitStatic(u32 max_frame_num,
                u32 min_frame_num, u32 size_y, u32 size_u,
                u32 size_v, u32 size_alpha, pixel_format_e format)
{
  CCedarFrameBufferManager *manager = new CCedarFrameBufferManager();
  if(!manager->FbmInit(max_frame_num, min_frame_num, size_y, size_u, size_v, size_alpha, format))
  {
    delete manager;
    manager = NULL;
  }

  return (Handle)manager;
}

Handle CCedarFrameBufferManager::FbmInitEx(u32 max_frame_num, u32 min_frame_num,
                u32 size_y[2], u32 size_u[2], u32 size_v[2], u32 size_alpha[2],
                _3d_mode_e out_3d_mode, pixel_format_e format, void* parent)

{
  Lock();
  

  //m_frames = std::max(min_frame_num, max_frame_num) + 10;

  m_frames = std::max((int)(min_frame_num + 3), 10);

  if(m_frames > MAX_FRAMES)
    return NULL;

  CCedarDecoder *decoder;

  DllLibVeCore.libve_io_ctrl(LIBVE_COMMAND_GET_PARENT, (u32)&decoder, parent);
  if(decoder == NULL)
    return NULL;;

  for(int i = 0; i < m_frames; i++)
  {
    FbmPicture_t *fbmPicture = (FbmPicture_t *)malloc(sizeof(FbmPicture_t));
    memset(fbmPicture, 0x0, sizeof(FbmPicture_t));
    vpicture_t *picture = &fbmPicture->picture;

    picture->id          = i;
    picture->size_y      = size_y[0];
    picture->size_u      = size_u[0];
    picture->size_v      = size_v[0];
    picture->size_alpha  = size_alpha[0];

    picture->size_y2     = size_y[1];
    picture->size_u2     = size_u[1];
    picture->size_v2     = size_v[1];
    picture->size_alpha2 = size_alpha[1];

    if(picture->size_y)
    {
      picture->y = (u8*)CCedarVEHwControll::MemPallocStatic(picture->size_y, 1024);
      if(!picture->y)
        return NULL;
    }
    if(picture->size_u)
    {
      picture->u = (u8*)CCedarVEHwControll::MemPallocStatic(picture->size_u, 1024);
      if(!picture->u)
        return NULL;
    }
    if(picture->size_v)
    {
      picture->v = (u8*)CCedarVEHwControll::MemPallocStatic(picture->size_v, 1024);
      if(!picture->v)
        return NULL;
    }
    if(picture->size_alpha)
    {
      picture->alpha = (u8*)CCedarVEHwControll::MemPallocStatic(picture->size_alpha, 1024);
      if(!picture->alpha)
        return NULL;
    }

    //fprintf(stderr, "picture->y 0x%08x picture->u 0x%08x picture->v 0x%08x\n",
    //    picture->y, picture->u, picture->v);
    fbmPicture->pictureState = STATE_FREE;

    m_buffers.push_back(fbmPicture);
    m_available.push_back(picture);
  }

  //fprintf(stderr, "%s::%s min_frame_num %d max_frame_num %d frames %d\n", CLASSNAME, __func__, 
  //    min_frame_num, max_frame_num, frames);
  m_open = true;

  m_readyFrames = 0;
  m_freeFrames = m_available.size();
  m_readyFrames = m_display.size();

  UnLock();
  return this;
}

Handle CCedarFrameBufferManager::FbmInitExStatic(u32 max_frame_num, u32 min_frame_num,
                u32 size_y[2], u32 size_u[2], u32 size_v[2], u32 size_alpha[2],
                _3d_mode_e out_3d_mode, pixel_format_e format, void* parent)
{
  CCedarFrameBufferManager *manager = new CCedarFrameBufferManager();
  if(!manager->FbmInitEx(max_frame_num, min_frame_num, size_y, size_u, size_v, size_alpha, out_3d_mode, format, parent))
  {
    delete manager;
    manager = NULL;
  }

  return (Handle)manager;
}

void CCedarFrameBufferManager::Flush()
{
  LockDisplay();
  m_display.clear();
  m_readyFrames = m_display.size();
  UnLockDisplay();

  Lock();
  m_available.clear();

  for(unsigned int i = 0; i < m_buffers.size(); i++)
  {
    FbmPicture_t *fbmPicture = m_buffers[i];
    vpicture_t *picture = &fbmPicture->picture;

    SetFbmPictureState(picture, STATE_FREE);
    m_available.push_back(picture);
  }
  m_freeFrames = m_available.size();
  UnLock();
}

int CCedarFrameBufferManager::MaxFrames()
{
  return m_frames;
}

int CCedarFrameBufferManager::ReadyFrames()
{
  return m_readyFrames;
}

int CCedarFrameBufferManager::FreeFrames()
{
  return m_freeFrames;
}

void CCedarFrameBufferManager::Lock()
{
  pthread_mutex_lock(&m_lock);
}

void CCedarFrameBufferManager::UnLock()
{
  pthread_mutex_unlock(&m_lock);
}

void CCedarFrameBufferManager::LockDisplay()
{
  pthread_mutex_lock(&m_lockDisplay);
}

void CCedarFrameBufferManager::UnLockDisplay()
{
  pthread_mutex_unlock(&m_lockDisplay);
}

#endif
