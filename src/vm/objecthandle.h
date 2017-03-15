// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "../gc/objecthandle.h"

// #define ObzjectFromHandle(handle)            ObjectToOBJECTREF(GCHeapUtilities::GetGCHandleTable()->ObjectFromHandle(handle))


// void DestroyPinningHandle(OBJECTHANDLE handle)
// {
//     GCHeapUtilities::GetGCHandleTable()->DestroyPinningHandle(handle);
// }

// typedef Wrapper<OBJECTHANDLE, DoNothing<OBJECTHANDLE>, DestroyPinningHandle, NULL> PinningHandleHolder;