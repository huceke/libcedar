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

#define CLASSNAME "CCedarDecoder"

#include "CedarDecoder.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdio.h>
#include "CedarBitstreamBufferManager.h"
#include "CedarFrameBufferManager.h"
#include "CedarVEHwControll.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LIBVECORE       "/usr/lib/libvecore.so"
#define LIBAVHEAP       "/usr/lib/libavheap.so"

libvecore DllLibVeCore;

__attribute__ ((visibility("default"))) IVBV_t IVBV;
__attribute__ ((visibility("default"))) IFBM_t IFBM;
__attribute__ ((visibility("default"))) IOS_t IOS;
__attribute__ ((visibility("default"))) IVEControl_t IVE;

void __attribute__((constructor)) library_init()
{
  IFBM.fbm_release        = CCedarFrameBufferManager::FbmReleaseStatic;
  IFBM.fbm_request_frame  = CCedarFrameBufferManager::FbmRequestFrameStatic;
  IFBM.fbm_return_frame   = CCedarFrameBufferManager::FbmReturnFrameStatic;
  IFBM.fbm_share_frame    = CCedarFrameBufferManager::FbmShareFrameStatic;
  IFBM.fbm_init_ex        = CCedarFrameBufferManager::FbmInitExStatic;

  IVBV.vbv_request_bitstream_frame  = &CCedarBitstreamBufferManager::VbvRequestBitstreamFrameStatic;
  IVBV.vbv_return_bitstream_frame   = &CCedarBitstreamBufferManager::VbVReturnBitstreamFrameStatic;
  IVBV.vbv_flush_bitstream_frame    = &CCedarBitstreamBufferManager::VbvFlushBitstreamFrameStatic;
  IVBV.vbv_get_base_addr            = &CCedarBitstreamBufferManager::VbvGetBaseAddrStatic;
  IVBV.vbv_get_size                 = &CCedarBitstreamBufferManager::VbvGetSizeStatic;

  IOS.mem_alloc             = &CCedarVEHwControll::MemAllocStatic;
  IOS.mem_free              = &CCedarVEHwControll::MemFreeStatic;
  IOS.mem_palloc            = &CCedarVEHwControll::MemPallocStatic;
  IOS.mem_pfree             = &CCedarVEHwControll::MemPfreeStatic;
  IOS.mem_set               = &CCedarVEHwControll::MemSetStatic;
  IOS.mem_cpy               = &CCedarVEHwControll::MemCpyStatic;
  IOS.mem_flush_cache       = &CCedarVEHwControll::MemFlushCacheStatic;
  IOS.mem_get_phy_addr      = &CCedarVEHwControll::MemGetPhyAddrStatic;
  IOS.sys_print             = &CCedarVEHwControll::SysPrintStatic;
  IOS.sys_sleep             = &CCedarVEHwControll::SysSleepStatic;

  IVE.ve_reset_hardware     = &CCedarVEHwControll::VeResetHardwareStatic;
  IVE.ve_enable_clock       = &CCedarVEHwControll::VeEnableClockStatic;
  IVE.ve_enable_intr        = &CCedarVEHwControll::VeEnableIntrStatic;
  IVE.ve_wait_intr          = &CCedarVEHwControll::VeWaitIntrStatic;
  IVE.ve_get_reg_base_addr  = &CCedarVEHwControll::VeGetRegBaseAddrStatic;
  IVE.ve_get_memtype        = &CCedarVEHwControll::VeGetMemTypeStatic;

  memset(&DllLibVeCore, 0x0, sizeof(libvecore));

  DllLibVeCore.handle_vecore = dlopen(LIBVECORE, RTLD_LAZY);
  if(!DllLibVeCore.handle_vecore)
  {
    printf("cedardecoder : error open %s : %s\n", LIBVECORE, dlerror());
  }
  else
  {
    DllLibVeCore.libve_set_ive  = (libve_set_ive_t)dlsym(DllLibVeCore.handle_vecore, "libve_set_ive");
    DllLibVeCore.libve_set_ios  = (libve_set_ios_t)dlsym(DllLibVeCore.handle_vecore, "libve_set_ios");
    DllLibVeCore.libve_set_ifbm = (libve_set_ifbm_t)dlsym(DllLibVeCore.handle_vecore, "libve_set_ifbm");
    DllLibVeCore.libve_set_ivbv = (libve_set_ivbv_t)dlsym(DllLibVeCore.handle_vecore, "libve_set_ivbv");

    DllLibVeCore.libve_open     = (libve_open_t)dlsym(DllLibVeCore.handle_vecore, "libve_open");
    DllLibVeCore.libve_close    = (libve_close_t)dlsym(DllLibVeCore.handle_vecore, "libve_close");
    DllLibVeCore.libve_reset    = (libve_reset_t)dlsym(DllLibVeCore.handle_vecore, "libve_reset");
    DllLibVeCore.libve_flush    = (libve_flush_t)dlsym(DllLibVeCore.handle_vecore, "libve_flush");
    DllLibVeCore.libve_decode   = (libve_decode_t)dlsym(DllLibVeCore.handle_vecore, "libve_decode");
    DllLibVeCore.libve_set_vbv  = (libve_set_vbv_t)dlsym(DllLibVeCore.handle_vecore, "libve_set_vbv");
    DllLibVeCore.libve_get_fbm  = (libve_get_fbm_t)dlsym(DllLibVeCore.handle_vecore, "libve_get_fbm");
    DllLibVeCore.libve_io_ctrl  = (libve_io_ctrl_t)dlsym(DllLibVeCore.handle_vecore, "libve_io_ctrl");

    if(/*!DllLibVeCore.libve_set_ive ||
        !DllLibVeCore.libve_set_ios ||
        !DllLibVeCore.libve_set_ifbm ||
        !DllLibVeCore.libve_set_ivbv ||*/
        !DllLibVeCore.libve_open ||
        !DllLibVeCore.libve_close ||
        !DllLibVeCore.libve_reset ||
        !DllLibVeCore.libve_flush ||
        !DllLibVeCore.libve_decode ||
        !DllLibVeCore.libve_set_vbv ||
        !DllLibVeCore.libve_get_fbm ||
        !DllLibVeCore.libve_io_ctrl)
    {
      printf("cedardecoder : error loading %s\n", LIBVECORE);
      dlclose(DllLibVeCore.handle_vecore);
      memset(&DllLibVeCore, 0x0, sizeof(libvecore));
    }

    if(DllLibVeCore.libve_set_ive)
    {
      // set method pointers first.
      DllLibVeCore.libve_set_ive(&IVE);
      DllLibVeCore.libve_set_ios(&IOS);
      DllLibVeCore.libve_set_ifbm(&IFBM);
      DllLibVeCore.libve_set_ivbv(&IVBV);
    }
  }

  DllLibVeCore.handle_avheap = dlopen(LIBAVHEAP, RTLD_NOW);
  if(!DllLibVeCore.handle_avheap)
  {
    printf("cedardecoder : error open %s : %s\n", LIBAVHEAP, dlerror());
  }
  else
  {
    DllLibVeCore.av_heap_init  = (av_heap_init_t)dlsym(DllLibVeCore.handle_avheap, "av_heap_init");
    DllLibVeCore.av_heap_release  = (av_heap_release_t)dlsym(DllLibVeCore.handle_avheap, "av_heap_release");
    DllLibVeCore.av_heap_alloc  = (av_heap_alloc_t)dlsym(DllLibVeCore.handle_avheap, "av_heap_alloc");
    DllLibVeCore.av_heap_free  = (av_heap_free_t)dlsym(DllLibVeCore.handle_avheap, "av_heap_free");
    DllLibVeCore.av_heap_physic_addr  = (av_heap_physic_addr_t)dlsym(DllLibVeCore.handle_avheap, "av_heap_physic_addr");
  }

  if(!DllLibVeCore.handle_vecore || !DllLibVeCore.handle_avheap)
  {
    if(DllLibVeCore.handle_vecore)
      dlclose(DllLibVeCore.handle_vecore);
    if(DllLibVeCore.handle_avheap)
      dlclose(DllLibVeCore.handle_avheap);
    memset(&DllLibVeCore, 0x0, sizeof(libvecore));
  }
}

void __attribute__((destructor)) library_uninit()
{
  if(DllLibVeCore.handle_vecore)
    dlclose(DllLibVeCore.handle_vecore);
  if(DllLibVeCore.handle_avheap)
    dlclose(DllLibVeCore.handle_avheap);
  memset(&DllLibVeCore, 0x0, sizeof(libvecore));
}

__attribute__ ((visibility("default"))) CCedarDecoder *AllocCedarDecoder()
{
  return new CCedarDecoder();
}

__attribute__ ((visibility("default"))) CCedarDecoder *FreeCedarDecoder(CCedarDecoder *decoder)
{
  delete decoder;
  return NULL;
}

__attribute__ ((visibility("default"))) unsigned int MemGetPhyAddr(unsigned int mem)
{
  return CCedarVEHwControll::MemGetPhyAddrStatic(mem);
}

__attribute__ ((visibility("default"))) void *MemPalloc(u32 size, u32 align)
{
  return CCedarVEHwControll::MemPallocStatic(size, align);
}

__attribute__ ((visibility("default"))) void MemPfree(void* p)
{
  CCedarVEHwControll::MemPfreeStatic(p);
}

__attribute__ ((visibility("default"))) void MemFlushCache(u8* mem, u32 size)
{
  CCedarVEHwControll::MemFlushCacheStatic(mem, size);
}

#ifdef __cplusplus
}
#endif

CCedarDecoder::CCedarDecoder()
{
  pthread_mutex_init(&m_lock, NULL);

  m_open = false;
  m_decoderHandle = 0;
  m_bufferManagerHandle = NULL;
  m_BitstreamBufferManager = NULL;
  m_FrameBufferManager = NULL;
  m_lastDisplayFrame = 0;

  g_CedarVEHwControll.Open();
  //g_CedarVEHwControll.VeEnableClock(true, 240 * 1000000);
}

CCedarDecoder::~CCedarDecoder()
{
  if(m_open)
    Close();

  m_open = false;

  //g_CedarVEHwControll.VeEnableClock(false, 160 * 1000000);
  g_CedarVEHwControll.Close();
  pthread_mutex_destroy(&m_lock);
}

void CCedarDecoder::Close()
{
  if(m_streamInfo.init_data)
    free(m_streamInfo.init_data);
  m_streamInfo.init_data = NULL;

  if(m_decoderHandle)
  {
    DllLibVeCore.libve_reset(0, m_decoderHandle);
    DllLibVeCore.libve_close(0, m_decoderHandle);
  }
  m_decoderHandle = NULL;

  if(m_BitstreamBufferManager)
    delete m_BitstreamBufferManager;
  m_BitstreamBufferManager = NULL;
  m_bufferManagerHandle = NULL;

  m_FrameBufferManager = NULL;

  m_open = false;
}

bool CCedarDecoder::Open(vstream_info_t *streamInfo, uint8_t *extradata, unsigned int extrasize)
{
  if(!DllLibVeCore.handle_vecore)
    return false;

  memset(&m_configInfo, 0x0, sizeof(vconfig_t));
  memset(&m_streamInfo, 0x0, sizeof(vstream_info_t));
  memcpy(&m_streamInfo, streamInfo, sizeof(vstream_info_t));

  if(extradata && extrasize)
  {
    m_streamInfo.init_data = (uint8_t*)malloc(extrasize);
    memcpy(m_streamInfo.init_data, extradata, extrasize);
    m_streamInfo.init_data_len = extrasize;
  }

  m_configInfo.max_video_width    = 1920;
  m_configInfo.max_video_height   = 1080;
  m_configInfo.max_memory_available = 0;

  m_BitstreamBufferManager = new CCedarBitstreamBufferManager();
  m_bufferManagerHandle = m_BitstreamBufferManager->Open(8*1024*1024, 1024);

  if(!m_bufferManagerHandle)
  {
    return false;
  }

  m_decoderHandle = DllLibVeCore.libve_open(&m_configInfo, &m_streamInfo, this);

  if(!m_decoderHandle)
  {
    return false;
  }

  DllLibVeCore.libve_set_vbv(m_bufferManagerHandle, m_decoderHandle);
  m_FrameBufferManager = (CCedarFrameBufferManager *)DllLibVeCore.libve_get_fbm(m_decoderHandle);

  Flush();

  //g_CedarVEHwControll.VeEnableClock(true, 320 * 1000000);

  m_open = true;

  return true;
}

vresult_e CCedarDecoder::Decode(bool drop, uint64_t pts)
{
  if(!m_open)
    return VRESULT_ERR_INVALID_PARAM;

  Lock();
  vresult_e res = DllLibVeCore.libve_decode(drop, 0, 0, m_decoderHandle);
  UnLock();

  return res;
}

void CCedarDecoder::Flush()
{
  Lock();
  if(m_decoderHandle)
    DllLibVeCore.libve_flush(0, m_decoderHandle);

  if(m_BitstreamBufferManager)
    m_BitstreamBufferManager->Flush();
  if(m_FrameBufferManager)
    m_FrameBufferManager->Flush();
  UnLock();
}

void CCedarDecoder::Lock()
{
  pthread_mutex_lock(&m_lock);
}

void CCedarDecoder::UnLock()
{
  pthread_mutex_unlock(&m_lock);
}

vpicture_t *CCedarDecoder::GetDisplayFrame()
{
  vpicture_t *picture = NULL;

  if(!m_FrameBufferManager)
    m_FrameBufferManager = (CCedarFrameBufferManager *)DllLibVeCore.libve_get_fbm(m_decoderHandle);

  if(m_FrameBufferManager)
    picture = m_FrameBufferManager->FbmGetDisplayFrame();

  return picture;
}

void CCedarDecoder::ReturnDisplayFrame(vpicture_t *picture)
{
  if(m_FrameBufferManager)
    m_FrameBufferManager->FbmReturnDisplayFrame(picture, (Handle)&m_FrameBufferManager);
}

vstream_data_t *CCedarDecoder::AllocateBuffer(unsigned int size, u64 pts)
{
  return m_BitstreamBufferManager->VbvAllocateBuffer(size, pts);
}

void CCedarDecoder::AddBufferData(vstream_data_t *streamData, u8 *data, unsigned int size)
{
  m_BitstreamBufferManager->VbvAddBufferData(streamData, data, size);
}

void CCedarDecoder::AddBuffer(vstream_data_t *streamData)
{
  m_BitstreamBufferManager->VbvAddBuffer(streamData);
}

uint64_t CCedarDecoder::GetAVS()
{
  return g_CedarVEHwControll.VeGetAVS();
}

s32 CCedarDecoder::WaitIntr()
{
  return g_CedarVEHwControll.VeWaitIntr();
}

unsigned int CCedarDecoder::MemGetPhyAddr(unsigned int mem)
{
  return g_CedarVEHwControll.MemGetPhyAddrStatic(mem);
}

void *CCedarDecoder::MemPalloc(u32 size, u32 align)
{
  return g_CedarVEHwControll.MemPallocStatic(size, align);
}

void CCedarDecoder::MemPfree(void* p)
{
  g_CedarVEHwControll.MemPfreeStatic(p);
}

void CCedarDecoder::MemFlushCache(u8* mem, u32 size)
{
  g_CedarVEHwControll.MemFlushCache(mem, size);
}

int CCedarDecoder::MaxFrames()
{
  if(!m_FrameBufferManager)
    m_FrameBufferManager = (CCedarFrameBufferManager *)DllLibVeCore.libve_get_fbm(m_decoderHandle);

  if(m_FrameBufferManager)
    return m_FrameBufferManager->MaxFrames();
  else
    return 0;
}

int CCedarDecoder::ReadyFrames()
{
  if(!m_FrameBufferManager)
    m_FrameBufferManager = (CCedarFrameBufferManager *)DllLibVeCore.libve_get_fbm(m_decoderHandle);

  if(m_FrameBufferManager)
    return m_FrameBufferManager->ReadyFrames();
  else
    return 0;
}

int CCedarDecoder::FreeFrames()
{
  if(!m_FrameBufferManager)
    m_FrameBufferManager = (CCedarFrameBufferManager *)DllLibVeCore.libve_get_fbm(m_decoderHandle);

  if(m_FrameBufferManager)
    return m_FrameBufferManager->FreeFrames();
  else
    return 5;
}

int CCedarDecoder::ReadyBuffers()
{
  if(m_BitstreamBufferManager)
    return m_BitstreamBufferManager->ReadyBuffers();
  else
    return 0;
}

void CCedarDecoder::CheckFrameBufferManager()
{
  if(!m_FrameBufferManager && m_open)
    m_FrameBufferManager = (CCedarFrameBufferManager *)DllLibVeCore.libve_get_fbm(m_decoderHandle);
}

#endif

#ifdef __cplusplus
extern "C" {
#endif
__attribute__ ((visibility("default"))) int cedarv_f23_ic_version()
{
	return 1;
}
#ifdef __cplusplus
}
#endif
