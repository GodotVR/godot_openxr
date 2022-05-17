/************************************************************************************

Filename    :   fb_spatial_entity_query.h
Content     :   spatial entity interface.
Language    :   C99

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#pragma once

#include <openxr/fb_spatial_entity.h>

/*
  157 XR_FB_spatial_entity_query
*/

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef XR_FB_spatial_entity_query

#ifndef XR_FB_spatial_entity
#error "This extension depends XR_FB_spatial_entity which has not been defined"
#endif

// While experimental, the experimental version must be set to get extension definitions
#if defined(XR_FB_spatial_entity_query_EXPERIMENTAL_VERSION)
// Error if the chosen experimental version is beyond the latest defined in this header
#if XR_FB_spatial_entity_query_EXPERIMENTAL_VERSION < 1 || \
    2 < XR_FB_spatial_entity_query_EXPERIMENTAL_VERSION
#error "unknown experimental version for XR_FB_spatial_entity_query"
#endif

#define XR_FB_spatial_entity_query 1

#define XR_FBX1_spatial_entity_query_SPEC_VERSION 2
#define XR_FBX1_SPATIAL_ENTITY_QUERY_EXTENSION_NAME "XR_FBX1_spatial_entity_query"

#define XR_FBX2_spatial_entity_query_SPEC_VERSION 2
#define XR_FBX2_SPATIAL_ENTITY_QUERY_EXTENSION_NAME "XR_FBX2_spatial_entity_query"

#if XR_FB_spatial_entity_query_EXPERIMENTAL_VERSION == 1
#define XR_FB_spatial_entity_query_SPEC_VERSION XR_FBX1_spatial_entity_query_SPEC_VERSION
#define XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME XR_FBX1_SPATIAL_ENTITY_QUERY_EXTENSION_NAME
#else
#define XR_FB_spatial_entity_query_SPEC_VERSION XR_FBX2_spatial_entity_query_SPEC_VERSION
#define XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME XR_FBX2_SPATIAL_ENTITY_QUERY_EXTENSION_NAME
#endif

// This extension allows an application to query the spaces that have been previously shared
// or persisted onto the device
//
// There are several types of Query actions that can be performed.
//
// - XR_SPATIAL_ENTITY_QUERY_PREDICATE_LOAD_FB
//      Performs a simple query that returns a XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_RESULT_FB for
//      each XrSpace that was found during the query.  There will also be a final
//      XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_COMPLETE_FB event that is returned when all XrSpaces
//      have been returned.  This query type can be used when looking for a list of all
//      spaces that match a filter criteria.  Using this query does an implicit load on the
//      spsAnchorHandle for the spatial anchor


// Example Usage:
//    XrSpatialEntityStorageLocationInfoFB storageLocationInfo = {
//        XR_TYPE_SPATIAL_ENTITY_STORAGE_LOCATION_INFO_FB,
//        nullptr,
//        XR_SPATIAL_ENTITY_STORAGE_LOCATION_LOCAL_FB};
//
//    XrSpatialEntityQueryFilterSpaceTypeFB filterInfo = {
//        XR_TYPE_SPATIAL_ENTITY_QUERY_FILTER_SPACE_TYPE_FB,
//        &storageLocationInfo,
//        XR_SPATIAL_ENTITY_TYPE_SPATIAL_ANCHOR_FB};
//
//    XrSpatialEntityQueryInfoActionQueryFB queryInfo = {
//        XR_TYPE_SPATIAL_ENTITY_QUERY_INFO_ACTION_QUERY_FB,
//        nullptr,
//        MAX_PERSISTENT_SPACES,
//        0,
//        XR_SPATIAL_ENTITY_QUERY_PREDICATE_LOAD_FB,
//        (XrSpatialEntityQueryFilterBaseHeaderFB*)&filterInfo,
//        nullptr};
//
//    XrAsyncRequestIdFB requestId;
//    XrResult result = xrQuerySpatialEntityFB(
//        app.Session, (XrSpatialEntityQueryInfoBaseHeaderFB*)&queryInfo, &requestId));
//

// Query Info Structs
static const XrStructureType XR_TYPE_SPATIAL_ENTITY_QUERY_INFO_ACTION_QUERY_FB =
    (XrStructureType)1000156000;

// Query Filters Structs
static const XrStructureType XR_TYPE_SPATIAL_ENTITY_QUERY_FILTER_SPACE_TYPE_FBX1 =
    (XrStructureType)1000156050;
static const XrStructureType XR_TYPE_SPATIAL_ENTITY_QUERY_FILTER_IDS_FBX1 =
    (XrStructureType)1000156051;

// Events
static const XrStructureType XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_RESULT_FBX1 =
    (XrStructureType)1000156100;
static const XrStructureType XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_COMPLETE_FBX1 =
    (XrStructureType)1000156101;

// Type of query being performed.
typedef enum XrSpatialEntityQueryPredicateFB {
    XR_SPATIAL_ENTITY_QUERY_PREDICATE_LOAD_FB = 0, // returns XrSpaces
        XR_SPATIAL_ENTITY_QUERY_PREDICATE_MAX_ENUM_FB = 0x7FFFFFFF
} XrSpatialEntityQueryPredicateFB;

// Query Filters
typedef struct XR_MAY_ALIAS XrSpatialEntityQueryFilterBaseHeaderFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
} XrSpatialEntityQueryFilterBaseHeaderFB;

// May be used to query the system to find all spaces of a particular type
typedef struct XrSpatialEntityQueryFilterSpaceTypeFBX1 {
    XrStructureType type; // XR_TYPE_SPATIAL_ENTITY_QUERY_FILTER_SPACE_TYPE_FBX1
    const void* XR_MAY_ALIAS next;
    XrSpatialEntityTypeFBX1 spaceType;
} XrSpatialEntityQueryFilterSpaceTypeFBX1;

// May be used to query the system to find all spaces that match the uuids provided
// in the filter info
typedef struct XrSpatialEntityQueryFilterIdsFBX1 {
    XrStructureType type; // XR_TYPE_SPATIAL_ENTITY_QUERY_FILTER_IDS_FBX1
    const void* XR_MAY_ALIAS next;
    XrSpatialEntityUuidFBX1* uuids;
    uint32_t numIds;
} XrSpatialEntityQueryFilterIdsFBX1;

// Query Info
typedef struct XR_MAY_ALIAS XrSpatialEntityQueryInfoBaseHeaderFB {
    XrStructureType type;
    const void* XR_MAY_ALIAS next;
} XrSpatialEntityQueryInfoBaseHeaderFB;

// May be used to query for spaces and perform a specific action on the spaces returned.
// The available actions can be found in XrSpatialEntityQueryPredicateFB.
// The filter info provided to the filter member of the struct will be used as an inclusive
// filter.  All spaces that match this criteria will be included in the results returned.
// The filter info provided to the excludeFilter member of the struct will be used to exclude
// spaces from the results returned from the filter.  This is to allow for a more selective
// style query
typedef struct XrSpatialEntityQueryInfoActionQueryFB {
    XrStructureType type; // XR_TYPE_SPATIAL_ENTITY_QUERY_INFO_ACTION_QUERY_FB
    const void* XR_MAY_ALIAS next; // string multiple queries together using next
    int32_t maxQuerySpaces;
    XrDuration timeout;
    XrSpatialEntityQueryPredicateFB queryAction; // What -> type of query to be performed
    const XrSpatialEntityQueryFilterBaseHeaderFB* filter; // Which -> which info we are querying for
    const XrSpatialEntityQueryFilterBaseHeaderFB*
        excludeFilter; // exclude specific results from query
} XrSpatialEntityQueryInfoActionQueryFB;

// Returned via XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_RESULT_FBX1 when a XrSpace is found matching the
// data provided by the filter info of the query
typedef struct XrEventSpatialEntityQueryResultFBX1 {
    XrStructureType type; // XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_RESULT_FBX1
    const void* XR_MAY_ALIAS next;
    XrAsyncRequestIdFB request;
    XrSpace space;
    XrSpatialEntityUuidFBX1 uuid;
} XrEventSpatialEntityQueryResultFBX1;

// When a query has completely finished this event will be returned
typedef struct XrEventSpatialEntityQueryCompleteFB {
    XrStructureType type; // XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_COMPLETE_FB
    const void* XR_MAY_ALIAS next;
    XrResult result;
    int32_t numSpacesFound;
    XrAsyncRequestIdFB request;
} XrEventSpatialEntityQueryCompleteFB;

typedef XrResult(XRAPI_PTR* PFN_xrQuerySpatialEntityFB)(
    XrSession session,
    const XrSpatialEntityQueryInfoBaseHeaderFB* info,
    XrAsyncRequestIdFB* request);


#ifndef XR_NO_PROTOTYPES
#ifdef XR_EXTENSION_PROTOTYPES

XRAPI_ATTR XrResult XRAPI_CALL xrQuerySpatialEntityFB(
    XrSession session,
    const XrSpatialEntityQueryInfoBaseHeaderFB* info,
    XrAsyncRequestIdFB* request);


#endif /* XR_EXTENSION_PROTOTYPES */
#endif /* !XR_NO_PROTOTYPES */

// Functionality introduced in FBX2
#if XR_FB_spatial_entity_query_EXPERIMENTAL_VERSION >= 2

// Query Filters Structs
static const XrStructureType XR_TYPE_SPATIAL_ENTITY_QUERY_FILTER_IDS_FB =
    (XrStructureType)1000156053;

// Events
static const XrStructureType XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_RESULTS_FB =
    (XrStructureType)1000156102;
static const XrStructureType XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_COMPLETE_FB =
    XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_COMPLETE_FBX1;

// May be used to query the system to find all spaces that match the uuids provided
// in the filter info
typedef struct XrSpatialEntityQueryFilterIdsFB {
    XrStructureType type; // XR_TYPE_SPATIAL_ENTITY_QUERY_FILTER_IDS_FB
    const void* XR_MAY_ALIAS next;
    XrSpatialEntityUuidFB* uuids;
    uint32_t numIds;
} XrSpatialEntityQueryFilterIdsFB;


// Query result to be returned in the results array of XrEventSpatialEntityQueryResultsFB or as
// output from xrEnumerateSpatialEntityQueryResultsFB(). No type or next pointer included to save
// space in the results array.
typedef struct XrSpatialEntityQueryResultFB {
    XrSpace space;
    XrSpatialEntityUuidFB uuid;
} XrSpatialEntityQueryResultFB;

// Maximum number of results that a single XrEventSpatialEntityQueryResultsFB can hold.
#define XR_FB_SPATIAL_ENTITY_QUERY_MAX_RESULTS_PER_EVENT 128

// Returned when some number of query results are available.
typedef struct XrEventSpatialEntityQueryResultsFB {
    XrStructureType type; // XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_RESULTS_FB
    const void* XR_MAY_ALIAS next;
    XrAsyncRequestIdFB request;
    uint32_t numResults;
    XrSpatialEntityQueryResultFB results[XR_FB_SPATIAL_ENTITY_QUERY_MAX_RESULTS_PER_EVENT];
} XrEventSpatialEntityQueryResultsFB;

#endif // XR_FB_spatial_entity_query_EXPERIMENTAL_VERSION >= 2

// FBX1 Backwards compatibility
#if XR_FB_spatial_entity_query_EXPERIMENTAL_VERSION == 1
#define XR_TYPE_SPATIAL_ENTITY_QUERY_FILTER_SPACE_TYPE_FB \
    XR_TYPE_SPATIAL_ENTITY_QUERY_FILTER_SPACE_TYPE_FBX1
#define XR_TYPE_SPATIAL_ENTITY_QUERY_FILTER_IDS_FB XR_TYPE_SPATIAL_ENTITY_QUERY_FILTER_IDS_FBX1
#define XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_RESULT_FB XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_RESULT_FBX1
#define XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_COMPLETE_FB \
    XR_TYPE_EVENT_SPATIAL_ENTITY_QUERY_COMPLETE_FBX1
#define XrSpatialEntityQueryFilterSpaceTypeFB XrSpatialEntityQueryFilterSpaceTypeFBX1
#define XrSpatialEntityQueryFilterIdsFB XrSpatialEntityQueryFilterIdsFBX1
#define XrEventSpatialEntityQueryResultFB XrEventSpatialEntityQueryResultFBX1

#endif // Backwards compatibility

#endif // defined(XR_FB_spatial_entity_query_EXPERIMENTAL_VERSION)

#endif // XR_FB_spatial_entity_query

#ifdef __cplusplus
}
#endif
