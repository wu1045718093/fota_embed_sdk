#include "MMI_features.h"
#include "custresdef.h"

/* Need this line to tell parser that XML start, must after all #include. */
<?xml version="1.0" encoding="UTF-8"?>

/* APP tag, include your app name defined in MMIDataType.h */
<APP id="APP_SMFOTA">
    <RECEIVER id="EVT_ID_SRV_NW_INFO_SERVICE_AVAILABILITY_CHANGED" proc="sm_event_handler"/>
    <RECEIVER id="EVT_ID_SRV_NW_INFO_LOCATION_CHANGED" proc="sm_event_handler"/>
</APP>
