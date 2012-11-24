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

#define CLASSNAME "CCedarBitstreamBufferManager"

#include "CedarBitstreamBufferManager.h"
#include "CedarFrameBufferManager.h"
#include "CedarVEHwControll.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

CCedarBitstreamBufferManager::CCedarBitstreamBufferManager()
{
  pthread_mutex_init(&m_lock, NULL);

  m_open = false;
  m_buffer = NULL;
  m_frameNumber = m_bufferSize = m_writePos = 0;
  m_frameCount = 0;
}

CCedarBitstreamBufferManager::~CCedarBitstreamBufferManager()
{
  if(m_open)
    Close();

  m_open = false;
}

void CCedarBitstreamBufferManager::VbvFreeBistreamFrame(vstream_data_t *streamData)
{
  if(streamData)
  {
    m_writeSize += streamData->length;
    free(streamData);
  }
}

void CCedarBitstreamBufferManager::Flush()
{
  Lock();

  while(!m_available.empty())
  {
    vstream_data_t *streamData = m_available.front();
    m_available.pop_front();

    VbvFreeBistreamFrame(streamData);
  }

  m_frameCount = m_available.size();

  UnLock();

  m_writePos = 0;
  m_frameNumber = 0;
}

void CCedarBitstreamBufferManager::Close()
{
  if(m_open)
    Flush();

  if(m_buffer)
    CCedarVEHwControll::MemPfreeStatic(m_buffer);
  m_buffer = NULL;

  m_writeSize = m_bufferSize = m_writePos = 0;
  m_open = false;
}

Handle CCedarBitstreamBufferManager::Open(unsigned int maxBufferSize, unsigned int maxFrameNumber)
{
  Lock();

  if(!maxBufferSize || !maxFrameNumber)
    return NULL;

  if(m_open)
    Close();

  m_maxFrameNumber = maxFrameNumber;
  m_writeSize = m_bufferSize = maxBufferSize;
  m_buffer = (u8*)CCedarVEHwControll::MemPallocStatic(m_bufferSize, 1024);

  if(!m_buffer)
    return NULL;

  m_open = true;

  UnLock();

  return this;
}

vstream_data_t *CCedarBitstreamBufferManager::VbvRequestBitstreamFrame()
{
  vstream_data_t *streamData = NULL;

  Lock();
  if(!m_available.empty())
  {
    streamData = m_available.front();
    m_available.pop_front();

    if((streamData->data + streamData->length) <= (m_buffer + m_bufferSize))
    {
      g_CedarVEHwControll.FlushCache(streamData->data, streamData->data + streamData->length);
    }
    else
    {
      u8* end = m_buffer + m_bufferSize;
      g_CedarVEHwControll.FlushCache(streamData->data, end);
      g_CedarVEHwControll.FlushCache(m_buffer, m_buffer + streamData->length - (end - streamData->data));
    }
  }

  m_frameCount = m_available.size();

  UnLock();

  return streamData;
}

vstream_data_t *CCedarBitstreamBufferManager::VbvRequestBitstreamFrameStatic(Handle vbv)
{
  CCedarBitstreamBufferManager *manager = (CCedarBitstreamBufferManager *)vbv;

  if(manager)
    return manager->VbvRequestBitstreamFrame();
  else
    return NULL;
}

void CCedarBitstreamBufferManager::VbVReturnBitstreamFrame(vstream_data_t* stream)
{
  fprintf(stderr, "%s::%s\n", CLASSNAME, __func__);
  Lock();
  m_available.push_front(stream);
  m_frameCount = m_available.size();
  UnLock();
}

void CCedarBitstreamBufferManager::VbVReturnBitstreamFrameStatic(vstream_data_t* stream, Handle vbv)
{
  CCedarBitstreamBufferManager *manager = (CCedarBitstreamBufferManager *)vbv;

  if(manager)
    manager->VbVReturnBitstreamFrame(stream);
}

void CCedarBitstreamBufferManager::VbvFlushBitstreamFrame(vstream_data_t* stream)
{
  Lock();
  if(stream)
  {
    VbvFreeBistreamFrame(stream);
  }
  UnLock();
}

void CCedarBitstreamBufferManager::VbvFlushBitstreamFrameStatic(vstream_data_t* stream, Handle vbv)
{
  CCedarBitstreamBufferManager *manager = (CCedarBitstreamBufferManager *)vbv;

  if(manager)
    manager->VbvFlushBitstreamFrame(stream);
}

u8* CCedarBitstreamBufferManager::VbvGetBaseAddr()
{
  return m_buffer;
}

u8* CCedarBitstreamBufferManager::VbvGetBaseAddrStatic(Handle vbv)
{
  CCedarBitstreamBufferManager *manager = (CCedarBitstreamBufferManager *)vbv;

  if(manager)
    return manager->VbvGetBaseAddr();
  else
    return NULL;
}

u32 CCedarBitstreamBufferManager::VbvGetSize()
{
  return m_bufferSize;
}

u32 CCedarBitstreamBufferManager::VbvGetSizeStatic(Handle vbv)
{
  CCedarBitstreamBufferManager *manager = (CCedarBitstreamBufferManager *)vbv;

  if(manager)
    return manager->VbvGetSize();
  else
    return 0;
}

void CCedarBitstreamBufferManager::VbvAddBuffer(vstream_data_t *streamData)
{
  if(!streamData)
    return;

  Lock();
  m_available.push_back(streamData);
  m_frameCount = m_available.size();
  UnLock();
}

vstream_data_t *CCedarBitstreamBufferManager::VbvAllocateBuffer(unsigned int size, u64 pts)
{
  if((size > m_bufferSize) || !size || (size > m_writeSize))
    return NULL;

  if(m_frameCount >= m_maxFrameNumber)
    return NULL;

  vstream_data_t *streamData = (vstream_data_t *)malloc(sizeof(vstream_data_t));
  memset(streamData, 0x0, sizeof(vstream_data_t));

  m_writeSize -= size;

  streamData->length = size;
  streamData->data = m_buffer + m_writePos;
  streamData->pts = pts;
  streamData->valid = 1;
  streamData->id = m_frameNumber;
  m_frameNumber = (m_frameNumber + 1) % ((m_maxFrameNumber));

  return streamData;
}

void CCedarBitstreamBufferManager::VbvAddBufferData(vstream_data_t *streamData, u8 *data_, unsigned int size)
{
  u8 *data = data_;

  if(!streamData || !data || !size)
    return;

  while(size)
  {
    unsigned int copy = 0;

    if((m_writePos + size) >= m_bufferSize)
      copy = m_bufferSize - m_writePos;
    else
      copy = size;

    memcpy(m_buffer + m_writePos, data, copy);

    size        -= copy;
    data        += copy;
    m_writePos  += copy;

    if(m_writePos >= m_bufferSize)
      m_writePos = 0;
  }
}

int CCedarBitstreamBufferManager::ReadyBuffers()
{
  return m_frameCount;
}

void CCedarBitstreamBufferManager::Lock()
{
  //pthread_mutex_lock(&m_lock);
}

void CCedarBitstreamBufferManager::UnLock()
{
  //pthread_mutex_unlock(&m_lock);
}

#endif
