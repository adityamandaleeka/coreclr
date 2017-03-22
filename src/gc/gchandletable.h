// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "gcinterface.h"

#ifndef GCHANDLETABLE_H_
#define GCHANDLETABLE_H_

class GCHandleTable : public IGCHandleTable
{
public:

    virtual bool Initialize();

    virtual void Shutdown();

    virtual HandleTableBucket* CreateHandleTableBucket(uint32_t appDomainIndex);

    virtual BOOL HandleAsyncPinHandles();

    virtual void RelocateAsyncPinHandles(HandleTableBucket *pSource, HandleTableBucket *pTarget);

    virtual void RemoveHandleTableBucket(HandleTableBucket *pBucket);

    virtual void DestroyHandleTableBucket(HandleTableBucket *pBucket);

    virtual BOOL ContainsHandle(HandleTableBucket *pBucket, OBJECTHANDLE handle);

    virtual void TraceRefCountedHandles(HANDLESCANPROC callback, uintptr_t lParam1, uintptr_t lParam2);

    ////////////
    virtual Object* ObjectFromHandle(OBJECTHANDLE handle);

    virtual void StoreObjectInHandle(OBJECTHANDLE handle, Object* object);

    virtual BOOL StoreFirstObjectInHandle(OBJECTHANDLE handle, Object* object);

    virtual BOOL ObjectHandleIsNull(OBJECTHANDLE handle);

    virtual void* InterlockedCompareExchangeObjectInHandle(OBJECTHANDLE handle, Object* objref, Object* oldObjref);

    virtual void DestroyHandle(OBJECTHANDLE handle);

    virtual void DestroyTypedHandle(OBJECTHANDLE handle);

    virtual BOOL IsHandleNullUnchecked(OBJECTHANDLE handle);

    virtual OBJECTHANDLE CreateTypedHandle(HHANDLETABLE table, Object* object, int type);

    virtual OBJECTHANDLE CreateDuplicateHandle(OBJECTHANDLE handle);

    virtual int GetCurrentThreadHomeHeapNumber();

    virtual OBJECTHANDLE CreateHandle(HHANDLETABLE table, Object* object);

    virtual OBJECTHANDLE CreateWeakHandle(HHANDLETABLE table, Object* object);

    virtual OBJECTHANDLE CreateShortWeakHandle(HHANDLETABLE table, Object* object);

    virtual OBJECTHANDLE CreateLongWeakHandle(HHANDLETABLE table, Object* object);

    virtual OBJECTHANDLE CreateStrongHandle(HHANDLETABLE table, Object* object);

    virtual OBJECTHANDLE CreatePinningHandle(HHANDLETABLE table, Object* object);

    virtual OBJECTHANDLE CreateSizedRefHandle(HHANDLETABLE table, Object* object);

    virtual OBJECTHANDLE CreateVariableHandle(HHANDLETABLE hTable, Object* object, uint32_t type);

    virtual OBJECTHANDLE CreateDependentHandle(HHANDLETABLE table, Object* primary, Object* secondary);

    // virtual OBJECTHANDLE CreateWinRTWeakHandle(HHANDLETABLE table, Object* object, IWeakReference* pWinRTWeakReference);

    virtual OBJECTHANDLE CreateWinRTWeakHandle(HHANDLETABLE table, Object* object, void* /* IWeakReference* */ pWinRTWeakReference);

    virtual void DestroyShortWeakHandle(OBJECTHANDLE handle);

    virtual void DestroyGlobalShortWeakHandle(OBJECTHANDLE handle);

    virtual void DestroyStrongHandle(OBJECTHANDLE handle);

    virtual void DestroyLongWeakHandle(OBJECTHANDLE handle);

    virtual void DestroyGlobalHandle(OBJECTHANDLE handle);

    virtual void DestroyGlobalStrongHandle(OBJECTHANDLE handle);

    virtual void DestroyDependentHandle(OBJECTHANDLE handle);

    virtual void DestroyAsyncPinningHandle(OBJECTHANDLE handle);

    virtual void DestroyRefcountedHandle(OBJECTHANDLE handle);

    virtual void DestroyWinRTWeakHandle(OBJECTHANDLE handle);

    virtual void DestroyPinningHandle(OBJECTHANDLE handle);

    virtual OBJECTHANDLE CreateGlobalHandle(Object* object);

    virtual OBJECTHANDLE CreateGlobalShortWeakHandle(Object* object);

    virtual OBJECTHANDLE CreateGlobalStrongHandle(Object* object);

    virtual OBJECTHANDLE CreateAsyncPinningHandle(HHANDLETABLE table, Object* object);

    virtual OBJECTHANDLE CreateGlobalWeakHandle(Object* object);

    virtual OBJECTHANDLE CreateRefcountedHandle(HHANDLETABLE table, Object* object);

    virtual void SetSecondaryForDependentHandle(OBJECTHANDLE handle, Object* secondary);

    virtual Object* GetSecondaryForDependentHandle(OBJECTHANDLE handle);

    virtual void* GetWeakReferenceForWinRTWeakHandle(OBJECTHANDLE handle);

    virtual void ResetObjectHandle(OBJECTHANDLE handle);

    virtual HandleTableBucket* GetFirstHandleTableBucketFromMap();

    virtual HHANDLETABLE GetHandleTableForHandle(OBJECTHANDLE handle);

};

#endif  // GCHANDLETABLE_H_
