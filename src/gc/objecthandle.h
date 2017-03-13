// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

/*
 * Wraps handle table to implement various handle types (Strong, Weak, etc.)
 *

 *
 */

#ifndef _OBJECTHANDLE_H
#define _OBJECTHANDLE_H

/*
 * include handle manager declarations
 */
#include "handletable.h"

#ifdef FEATURE_COMINTEROP
#include <weakreference.h>
#endif // FEATURE_COMINTEROP

/*
 * Convenience macros for accessing handles.  StoreFirstObjectInHandle is like
 * StoreObjectInHandle, except it only succeeds if transitioning from NULL to
 * non-NULL.  In other words, if this handle is being initialized for the first
 * time.
 */
// #define ObjectFromHandle(handle)                   HndFetchHandle(handle)
// #define ObzjectFromHandle(handle)                   HndFetchHandle(handle)

#define ObzjectFromHandle(handle)            ObjectToOBJECTREF(GCHeapUtilities::GetGCHandleTable()->ObjectFromHandle(handle))



#define StoreObjectInHandle(handle, object)        HndAssignHandle(handle, object)
// #define IntzerlockedCompareExchangeObjectInHandle(handle, object, oldObj)        HndInterlockedCompareExchangeHandle(handle, object, oldObj)
#define StoreFirstObjectInHandle(handle, object)   HndFirstAssignHandle(handle, object)
#define ObjectHandleIsNull(handle)                 HndIsNull(handle)
// #define IszHandleNullUnchecked(handle)              HndCheckForNullUnchecked(handle)

typedef DPTR(struct HandleTableMap) PTR_HandleTableMap;
typedef DPTR(struct HandleTableBucket) PTR_HandleTableBucket;
typedef DPTR(PTR_HandleTableBucket) PTR_PTR_HandleTableBucket;

struct HandleTableMap
{
    PTR_PTR_HandleTableBucket   pBuckets;
    PTR_HandleTableMap          pNext;
    uint32_t                    dwMaxIndex;
};

GVAL_DECL(HandleTableMap, g_HandleTableMap);

////////// THESE ARE ONLY USED IN GC/*
/*
 * Type mask definitions for HNDTYPE_VARIABLE handles.
 */
#define VHT_WEAK_SHORT              (0x00000100)  // avoid using low byte so we don't overlap normal types
#define VHT_WEAK_LONG               (0x00000200)  // avoid using low byte so we don't overlap normal types
#define VHT_STRONG                  (0x00000400)  // avoid using low byte so we don't overlap normal types
#define VHT_PINNED                  (0x00000800)  // avoid using low byte so we don't overlap normal types

#define IS_VALID_VHT_VALUE(flag)   ((flag == VHT_WEAK_SHORT) || \
                                    (flag == VHT_WEAK_LONG)  || \
                                    (flag == VHT_STRONG)     || \
                                    (flag == VHT_PINNED))

#ifndef DACCESS_COMPILE

inline void DezstroyHandle(OBJECTHANDLE handle)
{ 
    CONTRACTL
    {
        NOTHROW;
        GC_NOTRIGGER;
        MODE_ANY;
        CAN_TAKE_LOCK;
        SO_TOLERANT;
    }
    CONTRACTL_END;

    HndDestroyHandle(HndGetHandleTable(handle), HNDTYPE_DEFAULT, handle);
}

inline void DestroyPinningHandle(OBJECTHANDLE handle)
{ 
    WRAPPER_NO_CONTRACT;

    HndDestroyHandle(HndGetHandleTable(handle), HNDTYPE_PINNED, handle);
}

#ifndef FEATURE_REDHAWK
typedef Wrapper<OBJECTHANDLE, DoNothing<OBJECTHANDLE>, DestroyPinningHandle, NULL> PinningHandleHolder;
#endif


inline void DestroyAsyncPinningHandle(OBJECTHANDLE handle)
{ 
    WRAPPER_NO_CONTRACT;

    HndDestroyHandle(HndGetHandleTable(handle), HNDTYPE_ASYNCPINNED, handle);
}

#ifndef FEATURE_REDHAWK
typedef Wrapper<OBJECTHANDLE, DoNothing<OBJECTHANDLE>, DestroyAsyncPinningHandle, NULL> AsyncPinningHandleHolder;
#endif

// inline OBJECTHANDLE CreateSizedRefHandle(HHANDLETABLE table, OBJECTREF object)
// { 
//     WRAPPER_NO_CONTRACT;

//     return HndCreateHandle(table, HNDTYPE_SIZEDREF, object, (uintptr_t)0);
// }

// void DestroySizedRefHandle(OBJECTHANDLE handle);

#ifdef FEATURE_COMINTEROP
// inline OBJECTHANDLE CreateRefcountedHandle(HHANDLETABLE table, OBJECTREF object)
// { 
//     WRAPPER_NO_CONTRACT;

//     return HndCreateHandle(table, HNDTYPE_REFCOUNTED, object); 
// }

inline void DestroyRefcountedHandle(OBJECTHANDLE handle)
{ 
    WRAPPER_NO_CONTRACT;

    HndDestroyHandle(HndGetHandleTable(handle), HNDTYPE_REFCOUNTED, handle);
}

inline OBJECTHANDLE CreateWinRTWeakHandle(HHANDLETABLE table, OBJECTREF object, IWeakReference* pWinRTWeakReference)
{
    WRAPPER_NO_CONTRACT;
    _ASSERTE(pWinRTWeakReference != NULL);
    return HndCreateHandle(table, HNDTYPE_WEAK_WINRT, object, reinterpret_cast<uintptr_t>(pWinRTWeakReference));
}

void DestroyWinRTWeakHandle(OBJECTHANDLE handle);

#endif // FEATURE_COMINTEROP

#endif // !DACCESS_COMPILE

OBJECTREF GetDependentHandleSecondary(OBJECTHANDLE handle);

#ifndef DACCESS_COMPILE
OBJECTHANDLE CreateDependentHandle(HHANDLETABLE table, OBJECTREF primary, OBJECTREF secondary);
void SetDependentHandleSecondary(OBJECTHANDLE handle, OBJECTREF secondary);

inline void DestroyDependentHandle(OBJECTHANDLE handle)
{ 
    WRAPPER_NO_CONTRACT;

	HndDestroyHandle(HndGetHandleTable(handle), HNDTYPE_DEPENDENT, handle);
}
#endif // !DACCESS_COMPILE

#ifndef DACCESS_COMPILE

// OBJECTHANDLE CreateVariableHandle(HHANDLETABLE hTable, OBJECTREF object, uint32_t type);
// uint32_t     GetVariableHandleType(OBJECTHANDLE handle);
// void         UpdateVariableHandleType(OBJECTHANDLE handle, uint32_t type);
// uint32_t     CompareExchangeVariableHandleType(OBJECTHANDLE handle, uint32_t oldType, uint32_t newType);

inline void  DestroyVariableHandle(OBJECTHANDLE handle)
{
    WRAPPER_NO_CONTRACT;

    HndDestroyHandle(HndGetHandleTable(handle), HNDTYPE_VARIABLE, handle);
}

void GCHandleValidatePinnedObject(OBJECTREF obj);

/*
 * Holder for OBJECTHANDLE
 */

#ifndef FEATURE_REDHAWK
typedef Wrapper<OBJECTHANDLE, DoNothing<OBJECTHANDLE>, DezstroyHandle > OHWrapper;

class OBJECTHANDLEHolder : public OHWrapper
{
public:
    FORCEINLINE OBJECTHANDLEHolder(OBJECTHANDLE p = NULL) : OHWrapper(p)
    {
        LIMITED_METHOD_CONTRACT;
    }
    FORCEINLINE void operator=(OBJECTHANDLE p)
    {
        WRAPPER_NO_CONTRACT;

        OHWrapper::operator=(p);
    }
};
#endif

#ifdef FEATURE_COMINTEROP

typedef Wrapper<OBJECTHANDLE, DoNothing<OBJECTHANDLE>, DestroyRefcountedHandle> RefCountedOHWrapper;

class RCOBJECTHANDLEHolder : public RefCountedOHWrapper
{
public:
    FORCEINLINE RCOBJECTHANDLEHolder(OBJECTHANDLE p = NULL) : RefCountedOHWrapper(p)
    {
        LIMITED_METHOD_CONTRACT;
    }
    FORCEINLINE void operator=(OBJECTHANDLE p)
    {
        WRAPPER_NO_CONTRACT;

        RefCountedOHWrapper::operator=(p);
    }
};

#endif // FEATURE_COMINTEROP
/*
 * Convenience prototypes for using the global handles
 */

int GetCurrentThreadHomeHeapNumber();

// inline void DestroyGlobalHandle(OBJECTHANDLE handle)
// { 
//     WRAPPER_NO_CONTRACT;

//     HndDestroyHandle(HndGetHandleTable(handle), HNDTYPE_DEFAULT, handle);
// }

// inline void DestroyGlobalStrongHandle(OBJECTHANDLE handle)
// { 
//     WRAPPER_NO_CONTRACT;

//     HndDestroyHandle(HndGetHandleTable(handle), HNDTYPE_STRONG, handle);
// }

// #ifndef FEATURE_REDHAWK
// /////// this can be moved to the one place that uses it after STRESS_THREAD is gone
// // typedef Holder<OBJECTHANDLE,DoNothing<OBJECTHANDLE>,DestroyGlobalStrongHandle> GlobalStrongHandleHolder;
// #endif

inline void ResetOBJECTHANDLE(OBJECTHANDLE handle)
{
    WRAPPER_NO_CONTRACT;

    StoreObjectInHandle(handle, NULL);
}

// #ifndef FEATURE_REDHAWK
// typedef Holder<OBJECTHANDLE,DoNothing<OBJECTHANDLE>,ResetOBJECTHANDLE> ObjectInHandleHolder;
// #endif

/*
 * Table maintenance routines
 */
bool Ref_Initialize();
void Ref_Shutdown();
HandleTableBucket *Ref_CreateHandleTableBucket(ADIndex uADIndex);
BOOL Ref_HandleAsyncPinHandles();
void Ref_RelocateAsyncPinHandles(HandleTableBucket *pSource, HandleTableBucket *pTarget);
void Ref_RemoveHandleTableBucket(HandleTableBucket *pBucket);
void Ref_DestroyHandleTableBucket(HandleTableBucket *pBucket);
BOOL Ref_ContainHandle(HandleTableBucket *pBucket, OBJECTHANDLE handle);

/*
 * GC-time scanning entrypoints
 */
struct ScanContext;
struct DhContext;
void Ref_BeginSynchronousGC   (uint32_t uCondemnedGeneration, uint32_t uMaxGeneration);
void Ref_EndSynchronousGC     (uint32_t uCondemnedGeneration, uint32_t uMaxGeneration);

typedef void Ref_promote_func(class Object**, ScanContext*, uint32_t);

void Ref_TraceRefCountHandles(HANDLESCANPROC callback, uintptr_t lParam1, uintptr_t lParam2);
void Ref_TracePinningRoots(uint32_t condemned, uint32_t maxgen, ScanContext* sc, Ref_promote_func* fn);
void Ref_TraceNormalRoots(uint32_t condemned, uint32_t maxgen, ScanContext* sc, Ref_promote_func* fn);
void Ref_UpdatePointers(uint32_t condemned, uint32_t maxgen, ScanContext* sc, Ref_promote_func* fn);
void Ref_UpdatePinnedPointers(uint32_t condemned, uint32_t maxgen, ScanContext* sc, Ref_promote_func* fn);
DhContext *Ref_GetDependentHandleContext(ScanContext* sc);
bool Ref_ScanDependentHandlesForPromotion(DhContext *pDhContext);
void Ref_ScanDependentHandlesForClearing(uint32_t condemned, uint32_t maxgen, ScanContext* sc, Ref_promote_func* fn);
void Ref_ScanDependentHandlesForRelocation(uint32_t condemned, uint32_t maxgen, ScanContext* sc, Ref_promote_func* fn);
void Ref_ScanSizedRefHandles(uint32_t condemned, uint32_t maxgen, ScanContext* sc, Ref_promote_func* fn);
#ifdef FEATURE_REDHAWK
void Ref_ScanPointers(uint32_t condemned, uint32_t maxgen, ScanContext* sc, Ref_promote_func* fn);
#endif

typedef void (* handle_scan_fn)(Object** pRef, Object* pSec, uint32_t flags, ScanContext* context, BOOL isDependent);

void Ref_CheckReachable       (uint32_t uCondemnedGeneration, uint32_t uMaxGeneration, uintptr_t lp1);
void Ref_CheckAlive           (uint32_t uCondemnedGeneration, uint32_t uMaxGeneration, uintptr_t lp1);
void Ref_ScanHandlesForProfilerAndETW(uint32_t uMaxGeneration, uintptr_t lp1, handle_scan_fn fn);
void Ref_ScanDependentHandlesForProfilerAndETW(uint32_t uMaxGeneration, ScanContext * SC, handle_scan_fn fn);
void Ref_AgeHandles           (uint32_t uCondemnedGeneration, uint32_t uMaxGeneration, uintptr_t lp1);
void Ref_RejuvenateHandles(uint32_t uCondemnedGeneration, uint32_t uMaxGeneration, uintptr_t lp1);

void Ref_VerifyHandleTable(uint32_t condemned, uint32_t maxgen, ScanContext* sc);

#endif // DACCESS_COMPILE

#endif //_OBJECTHANDLE_H
