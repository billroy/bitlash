/*
 *      QueueUserAPCEx: Extending APCs on Windows Operating System (version 2.0)
 *      Copyright(C) 2004 Panagiotis E. Hadjidoukas
 *
 *      Contact Email: peh@hpclab.ceid.upatras.gr, xdoukas@ceid.upatras.gr
 *
 *      QueueUserAPCEx is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2 of the License, or (at your option) any later version.
 *
 *      QueueUserAPCEx is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with QueueUserAPCEx in the file COPYING.LIB;
 *      if not, write to the Free Software Foundation, Inc.,
 *      59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#if !defined( QUEUEUSERAPCEX_H )
#define QUEUEUSERAPCEX_H

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0501
#endif

#include <windows.h>

BOOL QueueUserAPCEx_Init(VOID);
BOOL QueueUserAPCEx_Fini(VOID);
DWORD QueueUserAPCEx(PAPCFUNC pfnApc, HANDLE hThread, DWORD dwData);

#endif	/* QUEUEUSERAPCEX_H */