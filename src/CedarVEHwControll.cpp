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

#if defined(HAVE_LIBCEDAR)

#define CLASSNAME "CCedarVEHwControll"

#include "CedarDecoder.h"
#include "CedarVEHwControll.h"
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stropts.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>

#define WALL_CLOCK_FREQ (50*1000)

CCedarVEHwControll g_CedarVEHwControll;

CCedarVEHwControll::CCedarVEHwControll()
{
  pthread_mutex_init(&m_lock, NULL);

  m_decoderHandle = -1;

  m_avsCntLast = 0;
  m_wallClock = 0;
}

CCedarVEHwControll::~CCedarVEHwControll()
{
  Close();
  pthread_mutex_destroy(&m_lock);
}

void CCedarVEHwControll::Close()
{
  if(m_decoderHandle >= 0)
  {
    ioctl(m_decoderHandle, IOCTL_PAUSE_AVS2, 0);

    ioctl(m_decoderHandle, IOCTL_ENGINE_REL, 0);
    DllLibVeCore.av_heap_release();

    munmap((void *)m_cedarEnvInfo.address_macc, 2048);
    close(m_decoderHandle);
  }

  m_decoderHandle = -1;
  m_open = false;
  memset(&m_cedarEnvInfo, 0x0, sizeof(cedarv_env_info_t));
}

bool CCedarVEHwControll::Open()
{
  if(m_open)
    Close();

  if(!DllLibVeCore.handle_avheap)
    return false;

  //fprintf(stderr, "CCedarVEHwControll::Open\n");
  memset(&m_cedarEnvInfo, 0x0, sizeof(cedarv_env_info_t));

  m_decoderHandle = open("/dev/cedar_dev", O_RDWR);
  if(m_decoderHandle == -1)
    return false;

  ioctl(m_decoderHandle, IOCTL_GET_ENV_INFO, (unsigned long)&m_cedarEnvInfo);
  m_cedarEnvInfo.address_macc = (unsigned int)mmap(NULL, 2048, PROT_READ | PROT_WRITE, MAP_SHARED, 
      m_decoderHandle, (int)m_cedarEnvInfo.address_macc);

  if(DllLibVeCore.av_heap_init(m_decoderHandle) != 0)
    return false;

  ioctl(m_decoderHandle, IOCTL_ENGINE_REQ, 0);

  m_open = true;

  return true;
}

void CCedarVEHwControll::VeResetHardware()
{
  Lock();
  ioctl(m_decoderHandle, IOCTL_RESET_VE, 0);
  UnLock();
}

void CCedarVEHwControll::VeResetHardwareStatic()
{
  g_CedarVEHwControll.VeResetHardware();
}

struct cedarv_engine_task_info {
  int task_prio;
  unsigned int frametime;
  unsigned int total_time;
};

uint64_t CCedarVEHwControll::VeGetAVS()
{
  unsigned int avsCntCurr;
  uint64_t val;

  avsCntCurr = ioctl(m_decoderHandle, IOCTL_GETVALUE_AVS2, 0);

  if((avsCntCurr ^ m_avsCntLast)>>31){ //wrap around
    m_wallClock += 0x00000000ffffffffLL;
  }
  m_avsCntLast = avsCntCurr;
  val = (m_wallClock + avsCntCurr) / (WALL_CLOCK_FREQ / 1000);

  return val;
}

void CCedarVEHwControll::VeEnableClock(u8 enable, u32 speed)
{
  u32 setspeed = speed / 1000000;
  //printf("CCedarVEHwControll::VeEnableClock %d %d\n", enable, setspeed);
  Lock();
  if(enable)
  {
    ioctl(m_decoderHandle, IOCTL_ENABLE_VE, 0);
    ioctl(m_decoderHandle, IOCTL_SET_VE_FREQ, setspeed);
  }
  else
  {
    //ioctl(m_decoderHandle, IOCTL_SET_VE_FREQ, setspeed);
    ioctl(m_decoderHandle, IOCTL_DISABLE_VE, 0);
  }
  UnLock();
}

void CCedarVEHwControll::VeEnableClockStatic(u8 enable, u32 speed)
{
  g_CedarVEHwControll.VeEnableClock(enable, speed);
}

void CCedarVEHwControll::VeEnableIntr(u8 enable)
{
}

void CCedarVEHwControll::VeEnableIntrStatic(u8 enable)
{
  g_CedarVEHwControll.VeEnableIntr(enable);
}

s32 CCedarVEHwControll::VeWaitIntr()
{
  Lock();
  int status = ioctl(m_decoderHandle, IOCTL_WAIT_VE, 2);
  UnLock();
  return status - 1;
}

s32 CCedarVEHwControll::VeWaitIntrStatic()
{
  return g_CedarVEHwControll.VeWaitIntr();
}

u32 CCedarVEHwControll::VeGetRegBaseAddr()
{
  return m_cedarEnvInfo.address_macc;
}

u32 CCedarVEHwControll::VeGetRegBaseAddrStatic()
{
  return g_CedarVEHwControll.VeGetRegBaseAddr();
}


memtype_e CCedarVEHwControll::VeGetMemType()
{
  return MEMTYPE_DDR3_32BITS;
}

memtype_e CCedarVEHwControll::VeGetMemTypeStatic()
{
  return g_CedarVEHwControll.VeGetMemType();
}

void *CCedarVEHwControll::MemAllocStatic(u32 size)
{
  return malloc(size);
}

void CCedarVEHwControll::MemFreeStatic(void* p)
{
  free(p);
}

void *CCedarVEHwControll::MemPallocStatic(u32 size, u32 align)
{
  return DllLibVeCore.av_heap_alloc((u32)size);
}

void CCedarVEHwControll::MemPfreeStatic(void* p)
{
  DllLibVeCore.av_heap_free(p);
}

void CCedarVEHwControll::MemSetStatic(void* mem, u32 value, u32 size)
{
  memset(mem, value, size);
}

void CCedarVEHwControll::MemCpyStatic(void* dst, void* src, u32 size)
{
  memcpy(dst, src, size);
}

void CCedarVEHwControll::MemFlushCache(u8* mem, u32 size)
{
}

void CCedarVEHwControll::MemFlushCacheStatic(u8* mem, u32 size)
{
  g_CedarVEHwControll.MemFlushCache(mem, size);
}

u32 CCedarVEHwControll::MemGetPhyAddrStatic(u32 virtual_addr)
{
  return (u32)DllLibVeCore.av_heap_physic_addr((void *)virtual_addr);
}

s32 CCedarVEHwControll::SysPrintStatic(u8* func, u32 line, ...)
{
  return 0;
}

void CCedarVEHwControll::SysSleepStatic(u32 ms)
{
  usleep(ms*1000);
}

void CCedarVEHwControll::FlushCache(u8 *start, u8 *end)
{
  if(!m_open)
    return;

  int args[2];

  args[0] = (int)start;
  args[1] = (int)end;

  if(ioctl(m_decoderHandle, IOCTL_FLUSH_CACHE, (void *)args) == -EFAULT)
    printf("error IOCTL_FLUSH_CACHE\n");
}

void CCedarVEHwControll::Lock()
{
  pthread_mutex_lock(&m_lock);
}

void CCedarVEHwControll::UnLock()
{
  pthread_mutex_unlock(&m_lock);
}

#endif
