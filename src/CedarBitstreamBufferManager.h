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

#ifndef _BITSTREAMBUFFERMANAGER_H_
#define _BITSTREAMBUFFERMANAGER_H_

#if defined(HAVE_LIBCEDAR)

#include <deque>

#include "libvecore/libve_typedef.h"
#include "libvecore/libve_adapter.h"
#include <pthread.h>

class CCedarBitstreamBufferManager
{
public:
  CCedarBitstreamBufferManager();
  virtual ~CCedarBitstreamBufferManager();
  virtual Handle Open(unsigned int maxBufferSize, unsigned int maxFrameNumber);
  virtual void VbvFreeBistreamFrame(vstream_data_t *streamData);
  virtual void Flush();
  virtual void Close();
  virtual vstream_data_t *VbvRequestBitstreamFrame();
  static vstream_data_t *VbvRequestBitstreamFrameStatic(Handle vbv);
  virtual void VbVReturnBitstreamFrame(vstream_data_t* stream);
  static void VbVReturnBitstreamFrameStatic(vstream_data_t* stream, Handle vbv);
  virtual void VbvFlushBitstreamFrame(vstream_data_t* stream);
  static void VbvFlushBitstreamFrameStatic(vstream_data_t* stream, Handle vbv);
  virtual u8* VbvGetBaseAddr();
  static u8* VbvGetBaseAddrStatic(Handle vbv);
  virtual u32 VbvGetSize();
  static u32 VbvGetSizeStatic(Handle vbv);
  virtual void VbvAddBuffer(vstream_data_t *streamData);
  virtual vstream_data_t *VbvAllocateBuffer(unsigned int size, u64 pts);
  virtual void VbvAddBufferData(vstream_data_t *streamData, u8 *data, unsigned int size);
  virtual int ReadyBuffers();
  virtual void Lock();
  virtual void UnLock();
  virtual unsigned int GetFrameNumber() { return m_frameNumber; };
private:
protected:
  bool          m_open;
  std::deque<vstream_data_t *> m_available;
  pthread_mutex_t m_lock;
  unsigned int m_writePos;
  unsigned int m_bufferSize;
  unsigned int m_writeSize;
  unsigned int m_frameNumber;
  unsigned int m_maxFrameNumber;
  unsigned int m_frameCount;
  u8*          m_buffer;
};

#endif

#endif
