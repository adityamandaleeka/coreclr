// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
// 

#include "common.h"
#include "gchandletable.h"
#include "objecthandle.h"

#ifdef FEATURE_COMINTEROP
#include <weakreference.h>
#endif

bool GCHandleTable::Initialize()
{
    return Ref_Initialize();
}

void GCHandleTable::Shutdown()
{
    Ref_Shutdown();
}

HandleTableBucket* GCHandleTable::CreateHandleTableBucket(uint32_t appDomainIndex)
{
#ifdef FEATURE_REDHAWK
    assert(!"Should not call GCHandleTable::CreateHandleTableBucket with FEATURE_REDHAWK defined!");
    return nullptr;
#else
    return Ref_CreateHandleTableBucket(ADIndex(appDomainIndex));
#endif
}

BOOL GCHandleTable::HandleAsyncPinHandles()
{
#ifdef FEATURE_REDHAWK
    assert(!"Should not call GCHandleTable::HandleAsyncPinHandles with FEATURE_REDHAWK defined!");
    return FALSE;
#else
    return Ref_HandleAsyncPinHandles();
#endif
}

void GCHandleTable::RelocateAsyncPinHandles(HandleTableBucket *pSource, HandleTableBucket *pTarget)
{
#ifdef FEATURE_REDHAWK
    assert(!"Should not call GCHandleTable::RelocateAsyncPinHandles with FEATURE_REDHAWK defined!");
    return;
#else
    Ref_RelocateAsyncPinHandles(pSource, pTarget);
#endif
}

void GCHandleTable::RemoveHandleTableBucket(HandleTableBucket *pBucket)
{
    Ref_RemoveHandleTableBucket(pBucket);
}

void GCHandleTable::DestroyHandleTableBucket(HandleTableBucket *pBucket)
{
    Ref_DestroyHandleTableBucket(pBucket);
}

BOOL GCHandleTable::ContainsHandle(HandleTableBucket *pBucket, OBJECTHANDLE handle)
{
    return Ref_ContainHandle(pBucket, handle);
}

void GCHandleTable::TraceRefCountedHandles(HANDLESCANPROC callback, uintptr_t lParam1, uintptr_t lParam2)
{
#ifdef FEATURE_COMINTEROP
    Ref_TraceRefCountHandles(callback, lParam1, lParam2);
#else
    assert(!"Should not call GCHandleTable::TraceRefCountedHandles without FEATURE_COMINTEROP defined!");
    return;
#endif
}

Object* GCHandleTable::ObjectFromHandle(OBJECTHANDLE handle)
{
    return OBJECTREFToObject(::HndFetchHandle(handle));
}

void GCHandleTable::StoreObjectInHandle(OBJECTHANDLE handle, Object* object)
{
    return ::HndAssignHandle(handle, ObjectToOBJECTREF(object));
}

BOOL GCHandleTable::StoreFirstObjectInHandle(OBJECTHANDLE handle, Object* object)
{
    return ::HndFirstAssignHandle(handle, ObjectToOBJECTREF(object));
}

BOOL GCHandleTable::ObjectHandleIsNull(OBJECTHANDLE handle)
{
    return ::HndIsNull(handle);
}

void* GCHandleTable::InterlockedCompareExchangeObjectInHandle(OBJECTHANDLE handle, Object* objref, Object* oldObjref)
{
    return ::HndInterlockedCompareExchangeHandle(handle, ObjectToOBJECTREF(objref), ObjectToOBJECTREF(oldObjref));
}

void GCHandleTable::DestroyHandle(OBJECTHANDLE handle)
{
    ::HndDestroyHandle(::HndGetHandleTable(handle), HNDTYPE_DEFAULT, handle); }

void GCHandleTable::DestroyTypedHandle(OBJECTHANDLE handle)
{
    ::HndDestroyHandleOfUnknownType(::HndGetHandleTable(handle), handle);
}

BOOL GCHandleTable::IsHandleNullUnchecked(OBJECTHANDLE handle) 
{
    return ::HndCheckForNullUnchecked(handle);
}

OBJECTHANDLE GCHandleTable::CreateTypedHandle(HHANDLETABLE table, Object* object, int type)
{
    return ::HndCreateHandle(table, type, ObjectToOBJECTREF(object));
}

// Create a new STRONG handle in the same table as an existing handle.
OBJECTHANDLE GCHandleTable::CreateDuplicateHandle(OBJECTHANDLE handle)
{
    return ::HndCreateHandle(::HndGetHandleTable(handle), HNDTYPE_DEFAULT, ObjectToOBJECTREF(ObjectFromHandle(handle)));
}

////// try to get out of needing this on the interface
int GCHandleTable::GetCurrentThreadHomeHeapNumber()
{
    return GzetCurrentThreadHomeHeapNumber();
}


// OBJECTHANDLE GCHandleTable::CreateZZZHandle(HHANDLETABLE table, Object* object, uint32_t handleType)
// {
//     return ::HndCreateHandle(table, handleType, ObjectToOBJECTREF(object));
// }

OBJECTHANDLE GCHandleTable::CreateHandle(HHANDLETABLE table, Object* object)
{
    return ::HndCreateHandle(table, HNDTYPE_DEFAULT, ObjectToOBJECTREF(object));
}

OBJECTHANDLE GCHandleTable::CreateWeakHandle(HHANDLETABLE table, Object* object)
{
    return ::HndCreateHandle(table, HNDTYPE_WEAK_DEFAULT, ObjectToOBJECTREF(object));
}

OBJECTHANDLE GCHandleTable::CreateShortWeakHandle(HHANDLETABLE table, Object* object)
{
    return ::HndCreateHandle(table, HNDTYPE_WEAK_SHORT, ObjectToOBJECTREF(object));
}

OBJECTHANDLE GCHandleTable::CreateLongWeakHandle(HHANDLETABLE table, Object* object)
{
    return ::HndCreateHandle(table, HNDTYPE_WEAK_LONG, ObjectToOBJECTREF(object));
}

OBJECTHANDLE GCHandleTable::CreateStrongHandle(HHANDLETABLE table, Object* object)
{
    return ::HndCreateHandle(table, HNDTYPE_STRONG, ObjectToOBJECTREF(object));
}

OBJECTHANDLE GCHandleTable::CreatePinningHandle(HHANDLETABLE table, Object* object)
{
    return ::HndCreateHandle(table, HNDTYPE_PINNED, ObjectToOBJECTREF(object));
}

OBJECTHANDLE GCHandleTable::CreateSizedRefHandle(HHANDLETABLE table, Object* object)
{
    return ::HndCreateHandle(table, HNDTYPE_SIZEDREF, ObjectToOBJECTREF(object), (uintptr_t)0);
}

OBJECTHANDLE GCHandleTable::CreateVariableHandle(HHANDLETABLE hTable, Object* object, uint32_t type)
{
    return CrzeateVariableHandle(hTable, ObjectToOBJECTREF(object), type);
}

OBJECTHANDLE GCHandleTable::CreateDependentHandle(HHANDLETABLE table, Object* primary, Object* secondary)
{
    OBJECTHANDLE handle = ::HndCreateHandle(table, HNDTYPE_DEPENDENT, ObjectToOBJECTREF(primary)); 
    SetSecondaryForDependentHandle(handle, secondary);
    return handle;
}

OBJECTHANDLE GCHandleTable::CreateWinRTWeakHandle(HHANDLETABLE table, Object* object, void* /* IWeakReference* */ pWinRTWeakReference)
{
#ifdef FEATURE_COMINTEROP
    return ::HndCreateHandle(table, HNDTYPE_WEAK_WINRT, ObjectToOBJECTREF(object), reinterpret_cast<uintptr_t>(pWinRTWeakReference));
#else
    assert(!"Should not call GCHandleTable::CreateWinRTWeakHandle without FEATURE_COMINTEROP defined!");
    return NULL;
#endif
}

void GCHandleTable::DestroyGlobalShortWeakHandle(OBJECTHANDLE handle)
{
    ::HndDestroyHandle(::HndGetHandleTable(handle), HNDTYPE_WEAK_SHORT, handle);
}

void GCHandleTable::DestroyShortWeakHandle(OBJECTHANDLE handle)
{
    ::HndDestroyHandle(::HndGetHandleTable(handle), HNDTYPE_WEAK_SHORT, handle);
}

void GCHandleTable::DestroyStrongHandle(OBJECTHANDLE handle)
{
    ::HndDestroyHandle(::HndGetHandleTable(handle), HNDTYPE_STRONG, handle);
}

void GCHandleTable::DestroyLongWeakHandle(OBJECTHANDLE handle)
{
    ::HndDestroyHandle(::HndGetHandleTable(handle), HNDTYPE_WEAK_LONG, handle);
}

void GCHandleTable::DestroyGlobalHandle(OBJECTHANDLE handle)
{
    ::HndDestroyHandle(::HndGetHandleTable(handle), HNDTYPE_DEFAULT, handle);
}

void GCHandleTable::DestroyGlobalStrongHandle(OBJECTHANDLE handle)
{
    ::HndDestroyHandle(::HndGetHandleTable(handle), HNDTYPE_STRONG, handle);
}

void GCHandleTable::DestroyDependentHandle(OBJECTHANDLE handle)
{
    ::HndDestroyHandle(::HndGetHandleTable(handle), HNDTYPE_DEPENDENT, handle);
}

void GCHandleTable::DestroyAsyncPinningHandle(OBJECTHANDLE handle)
{
    ::HndDestroyHandle(::HndGetHandleTable(handle), HNDTYPE_ASYNCPINNED, handle);
}

void GCHandleTable::DestroyRefcountedHandle(OBJECTHANDLE handle)
{
    ::HndDestroyHandle(::HndGetHandleTable(handle), HNDTYPE_REFCOUNTED, handle);
}

void GCHandleTable::DestroyWinRTWeakHandle(OBJECTHANDLE handle)
{
#ifdef FEATURE_COMINTEROP
    // Release the WinRT weak reference if we have one.  We're assuming that this will not reenter the
    // runtime, since if we are pointing at a managed object, we should not be using a HNDTYPE_WEAK_WINRT
    // but rather a HNDTYPE_WEAK_SHORT or HNDTYPE_WEAK_LONG.
    IWeakReference* pWinRTWeakReference = reinterpret_cast<IWeakReference*>(::HndGetHandleExtraInfo(handle));
    if (pWinRTWeakReference != nullptr)
    {
        pWinRTWeakReference->Release();
    }

    ::HndDestroyHandle(::HndGetHandleTable(handle), HNDTYPE_WEAK_WINRT, handle);
#else
    assert(!"Should not call GCHandleTable::DestroyWinRTWeakHandle without FEATURE_COMINTEROP defined!");
#endif
}

void GCHandleTable::DestroyPinningHandle(OBJECTHANDLE handle)
{
    ::HndDestroyHandle(::HndGetHandleTable(handle), HNDTYPE_PINNED, handle);
}

OBJECTHANDLE GCHandleTable::CreateGlobalHandle(Object* object)
{
    return ::HndCreateHandle(g_HandleTableMap.pBuckets[0]->pTable[GetCurrentThreadHomeHeapNumber()], HNDTYPE_DEFAULT, ObjectToOBJECTREF(object)); 
}

OBJECTHANDLE GCHandleTable::CreateGlobalShortWeakHandle(Object* object)
{
    return ::HndCreateHandle(g_HandleTableMap.pBuckets[0]->pTable[GetCurrentThreadHomeHeapNumber()], HNDTYPE_WEAK_SHORT, ObjectToOBJECTREF(object));
}

OBJECTHANDLE GCHandleTable::CreateGlobalStrongHandle(Object* object)
{
    return ::HndCreateHandle(g_HandleTableMap.pBuckets[0]->pTable[GetCurrentThreadHomeHeapNumber()], HNDTYPE_STRONG, ObjectToOBJECTREF(object)); 
}

OBJECTHANDLE GCHandleTable::CreateAsyncPinningHandle(HHANDLETABLE table, Object* object)
{
    return ::HndCreateHandle(table, HNDTYPE_ASYNCPINNED, ObjectToOBJECTREF(object));
}

OBJECTHANDLE GCHandleTable::CreateGlobalWeakHandle(Object* object)
{
    return ::HndCreateHandle(g_HandleTableMap.pBuckets[0]->pTable[GetCurrentThreadHomeHeapNumber()], HNDTYPE_WEAK_DEFAULT, ObjectToOBJECTREF(object));
}

OBJECTHANDLE GCHandleTable::CreateRefcountedHandle(HHANDLETABLE table, Object* object)
{
    return ::HndCreateHandle(table, HNDTYPE_REFCOUNTED, ObjectToOBJECTREF(object));
}

void GCHandleTable::SetSecondaryForDependentHandle(OBJECTHANDLE handle, Object* secondary)
{
    return SzetDependentHandleSecondary(handle, ObjectToOBJECTREF(secondary));
}

Object* GCHandleTable::GetSecondaryForDependentHandle(OBJECTHANDLE handle)
{
    return OBJECTREFToObject(GetDependentHandleSecondary(handle));
}

void* GCHandleTable::GetWeakReferenceForWinRTWeakHandle(OBJECTHANDLE handle)
{
    _ASSERTE(HandleFetchType(handle) == HNDTYPE_WEAK_WINRT);
    return (void*)::HndGetHandleExtraInfo(handle);
}

void GCHandleTable::ResetObjectHandle(OBJECTHANDLE handle)
{
    ::HndAssignHandle(handle, NULL);
}

HandleTableBucket* GCHandleTable::GetFirstHandleTableBucketFromMap()
{
    return g_HandleTableMap.pBuckets[0];
}

HHANDLETABLE GCHandleTable::GetHandleTableForHandle(OBJECTHANDLE handle)
{
    return ::HndGetHandleTable(handle);
}