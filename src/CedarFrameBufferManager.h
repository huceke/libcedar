/*
 *      Copyright (C) 2013 Edgar Hucek
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

#ifndef _FRAMEBUFFERMANAGER_H_
#define _FRAMEBUFFERMANAGER_H_

#if defined(HAVE_LIBCEDAR)

#include <vector>
#include <deque>

#include "libvecore/libve_typedef.h"
#include "libvecore/libve_adapter.h"
#include <pthread.h>

enum PictureState
{
  STATE_FREE        = 0,
  STATE_DECODER,
  STATE_SHARED,
  STATE_SHARED_FREE,
  STATE_DISPLAY,
  STATE_UNDEFINED
};

typedef struct FbmPicture_t
{
  vpicture_t picture;
  PictureState pictureState;
} _FbmPicture_t;

class CCedarFrameBufferManager
{
public:
  CCedarFrameBufferManager();
  virtual ~CCedarFrameBufferManager();
  virtual FbmPicture_t *GetFbmPicture(vpicture_t *picture);
  virtual vpicture_t* FbmGetDisplayFrame();
  virtual vpicture_t* FbmGetDisplayFrameStatic(Handle h);
  virtual void SetFbmPictureState(vpicture_t *picture, enum PictureState pictureState);
  virtual enum PictureState GetFbmPictureState(vpicture_t *picture);
  virtual void FbmRelease(Handle h, void* parent);
  static void FbmReleaseStatic(Handle h, void* parent);
  virtual vpicture_t* FbmRequestFrame(Handle h);
  static vpicture_t* FbmRequestFrameStatic(Handle h);
  virtual void FbmReturnDisplayFrame(vpicture_t* picture, Handle h);
  virtual void FbmReturnFrame(vpicture_t* picture, u8 valid, Handle h);
  static void FbmReturnFrameStatic(vpicture_t* picture, u8 valid, Handle h);
  virtual void FbmShareFrame(vpicture_t* picture, Handle h);
  static void FbmShareFrameStatic(vpicture_t* picture, Handle h);
  virtual Handle FbmInit(u32 max_frame_num,
                u32 min_frame_num, u32 size_y, u32 size_u,
                u32 size_v, u32 size_alpha, pixel_format_e format);
  static Handle FbmInitStatic(u32 max_frame_num,
                u32 min_frame_num, u32 size_y, u32 size_u,
                u32 size_v, u32 size_alpha, pixel_format_e format);
  virtual Handle FbmInitEx(u32 max_frame_num, u32 min_frame_num,
                u32 size_y[2], u32 size_u[2], u32 size_v[2], u32 size_alpha[2],
                _3d_mode_e out_3d_mode, pixel_format_e format, void* parent);
  static Handle FbmInitExStatic(u32 max_frame_num, u32 min_frame_num,
                u32 size_y[2], u32 size_u[2], u32 size_v[2], u32 size_alpha[2],
                _3d_mode_e out_3d_mode, pixel_format_e format, void* parent);
  virtual int MaxFrames();
  virtual int ReadyFrames();
  virtual int FreeFrames();
  virtual void Flush();
  virtual void Lock();
  virtual void UnLock();
  virtual void LockDisplay();
  virtual void UnLockDisplay();
private:
protected:
  bool          m_open;
  std::deque<vpicture_t *> m_display;
  std::deque<vpicture_t *> m_available;
  std::vector<FbmPicture_t *> m_buffers;
  pthread_mutex_t m_lock;
  pthread_mutex_t m_lockDisplay;
  int  m_frames;
  int  m_readyFrames;
  int  m_freeFrames;
};

#endif

#endif
