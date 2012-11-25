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

#ifndef _CEDARDECODER_H_
#define _CEDARDECODER_H_

#if defined(HAVE_LIBCEDAR)

#include <pthread.h>
#include <stdint.h>
#include "libvecore/libve.h"

class CCedarDecoder;
class CCedarFrameBufferManager;
class CCedarBitstreamBufferManager;

#ifdef __cplusplus
extern "C" {
#endif

typedef vresult_e (*libve_set_ive_t)(IVEControl_t* ive);
typedef vresult_e (*libve_set_ios_t)(IOS_t* ios);
typedef vresult_e (*libve_set_ifbm_t)(IFBM_t* ifbm);
typedef vresult_e (*libve_set_ivbv_t)(IVBV_t* ivbv);
typedef Handle    (*libve_open_t)(vconfig_t* config, vstream_info_t* stream_info, void* parent);
typedef vresult_e (*libve_close_t)(u8 flush_pictures, Handle libve);
typedef vresult_e (*libve_reset_t)(u8 flush_pictures, Handle libve);
typedef vresult_e (*libve_flush_t)(u8 flush_pictures, Handle libve);
typedef vresult_e (*libve_decode_t)(u8 keyframe_only, u8 skip_bframe, u64 cur_time, Handle libve);
typedef vresult_e (*libve_set_vbv_t)(Handle vbv, Handle libve);
typedef Handle    (*libve_get_fbm_t)(Handle libve);
typedef vresult_e (*libve_io_ctrl_t)(u32 cmd, u32 param, Handle libve);
typedef int       (*av_heap_init_t)(int fd_ve);
typedef void      (*av_heap_release_t)(void);
typedef void*     (*av_heap_alloc_t)(int size);
typedef void      (*av_heap_free_t)(void* mem);
typedef void*     (*av_heap_physic_addr_t)(void* vaddr);


typedef struct _libvecore
{
  void *handle_vecore;
  void *handle_avheap;
  libve_set_ive_t   libve_set_ive;
  libve_set_ios_t   libve_set_ios;
  libve_set_ifbm_t  libve_set_ifbm;
  libve_set_ivbv_t  libve_set_ivbv;
  libve_open_t      libve_open;
  libve_close_t     libve_close;
  libve_reset_t     libve_reset;
  libve_flush_t     libve_flush;
  libve_decode_t    libve_decode;
  libve_set_vbv_t   libve_set_vbv;
  libve_get_fbm_t   libve_get_fbm;
  libve_io_ctrl_t   libve_io_ctrl;
  av_heap_init_t    av_heap_init;
  av_heap_release_t av_heap_release;
  av_heap_alloc_t   av_heap_alloc;
  av_heap_free_t    av_heap_free;
  av_heap_physic_addr_t av_heap_physic_addr;
} libvecore;

#ifdef CEDAR_LIBRARY
extern libvecore DllLibVeCore;
#endif

__attribute__ ((visibility("default"))) CCedarDecoder *AllocCedarDecoder();
__attribute__ ((visibility("default"))) CCedarDecoder *FreeCedarDecoder(CCedarDecoder *decoder);
__attribute__ ((visibility("default"))) unsigned int MemGetPhyAddr(unsigned int mem);
__attribute__ ((visibility("default"))) void *MemPalloc(u32 size, u32 align);
__attribute__ ((visibility("default"))) void MemPfree(void* p);

#ifdef __cplusplus
}
#endif

class CCedarDecoder
{
public:
  CCedarDecoder();
  virtual ~CCedarDecoder();
  virtual bool Open(vstream_info_t *streamInfo, uint8_t *extradata, unsigned int extrasize);
  virtual void Close();
  virtual void Reset();
  virtual void Flush();
  virtual void Lock();
  virtual void UnLock();
  virtual vresult_e Decode(bool drop, uint64_t pts);
  virtual vpicture_t *GetDisplayFrame();
  virtual void ReturnDisplayFrame(vpicture_t *picture);
  virtual vstream_data_t *AllocateBuffer(unsigned int size, u64 pts);
  virtual void AddBufferData(vstream_data_t *streamData, u8 *data, unsigned int size);
  virtual void AddBuffer(vstream_data_t *streamData);
  virtual uint64_t GetAVS();
  virtual s32 WaitIntr();
  virtual unsigned int MemGetPhyAddr(unsigned int mem);
  virtual void *MemPalloc(u32 size, u32 align);
  virtual void MemPfree(void* p);
  virtual void MemFlushCache(u8* mem, u32 size);
  virtual int  MaxFrames();
  virtual int  ReadyFrames();
  virtual int  FreeFrames();
  virtual int  ReadyBuffers();
  virtual void SetFrameBufferManager(CCedarFrameBufferManager *FrameBufferManager) { m_FrameBufferManager = FrameBufferManager; };
  virtual void CheckFrameBufferManager();
  virtual int64_t LastDisplayFrame() { return m_lastDisplayFrame; };
  virtual void    LastDisplayFrame(int64_t lastDisplayFrame) { m_lastDisplayFrame = lastDisplayFrame; };
private:
protected:
  bool          m_open;
  pthread_mutex_t m_lock;
  vconfig_t       m_configInfo;
  vstream_info_t  m_streamInfo;
  Handle          m_decoderHandle;
  Handle          m_bufferManagerHandle;
  CCedarFrameBufferManager *m_FrameBufferManager;
  CCedarBitstreamBufferManager *m_BitstreamBufferManager;
  int64_t         m_lastDisplayFrame;
};

#endif

#endif
