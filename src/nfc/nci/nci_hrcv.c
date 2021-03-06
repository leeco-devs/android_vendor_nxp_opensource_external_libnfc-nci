/******************************************************************************
 *
 *  Copyright (C) 2010-2014 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
/******************************************************************************
 *
 *  The original Work has been changed by NXP Semiconductors.
 *
 *  Copyright (C) 2015 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  This file contains function of the NFC unit to receive/process NCI
 *  commands.
 *
 ******************************************************************************/
#include <string.h>
#include "nfc_target.h"
#include "bt_types.h"
#include "gki.h"

#include "nci_defs.h"
#include "nci_hmsgs.h"
#include "nfc_api.h"
#include "nfc_int.h"
#if (NXP_EXTNS == TRUE)
#include "nfa_sys.h"
#endif
#include "nfa_ee_int.h"
/*******************************************************************************
**
** Function         nci_proc_core_rsp
**
** Description      Process NCI responses in the CORE group
**
** Returns          true-caller of this function to free the GKI buffer p_msg
**
*******************************************************************************/
bool nci_proc_core_rsp(NFC_HDR* p_msg) {
  uint8_t* p;
  uint8_t* pp, len, op_code;
  bool free = true;
  uint8_t* p_old = nfc_cb.last_cmd;

  /* find the start of the NCI message and parse the NCI header */
  p = (uint8_t*)(p_msg + 1) + p_msg->offset;
  pp = p + 1;
  NCI_MSG_PRS_HDR1(pp, op_code);
  NFC_TRACE_DEBUG1("nci_proc_core_rsp opcode:0x%x", op_code);
  len = *pp++;

  /* process the message based on the opcode and message type */
  switch (op_code) {
    case NCI_MSG_CORE_RESET:
      nfc_ncif_proc_reset_rsp(pp, false);
      break;

    case NCI_MSG_CORE_INIT:
      nfc_ncif_proc_init_rsp(p_msg);
      free = false;
      break;

    case NCI_MSG_CORE_GET_CONFIG:
      nfc_ncif_proc_get_config_rsp(p_msg);
      break;

    case NCI_MSG_CORE_SET_CONFIG:
      nfc_ncif_set_config_status(pp, len);
      break;

    case NCI_MSG_CORE_CONN_CREATE:
      nfc_ncif_proc_conn_create_rsp(p, p_msg->len, *p_old);
      break;

    case NCI_MSG_CORE_CONN_CLOSE:
      nfc_ncif_report_conn_close_evt(*p_old, *pp);
      break;

    case NCI_MSG_CORE_SET_POWER_SUB_STATE:
      nfc_ncif_event_status(NFC_SET_POWER_SUB_STATE_REVT, *pp);
      break;
    default:
      NFC_TRACE_ERROR1("unknown opcode:0x%x", op_code);
      break;
  }

  return free;
}

/*******************************************************************************
**
** Function         nci_proc_core_ntf
**
** Description      Process NCI notifications in the CORE group
**
** Returns          void
**
*******************************************************************************/
void nci_proc_core_ntf(NFC_HDR* p_msg) {
  uint8_t* p;
  uint8_t* pp, len, op_code;
  uint8_t conn_id;

  /* find the start of the NCI message and parse the NCI header */
  p = (uint8_t*)(p_msg + 1) + p_msg->offset;
  pp = p + 1;
  NCI_MSG_PRS_HDR1(pp, op_code);
  NFC_TRACE_DEBUG1("nci_proc_core_ntf opcode:0x%x", op_code);
  len = *pp++;

  /* process the message based on the opcode and message type */
  switch (op_code) {
    case NCI_MSG_CORE_RESET:
      nfc_ncif_proc_reset_rsp(pp, true);
      break;

    case NCI_MSG_CORE_GEN_ERR_STATUS:
      /* process the error ntf */
      /* in case of timeout: notify the static connection callback */
      nfc_ncif_event_status(NFC_GEN_ERROR_REVT, *pp);
      nfc_ncif_error_status(NFC_RF_CONN_ID, *pp);
      break;

    case NCI_MSG_CORE_INTF_ERR_STATUS:
      conn_id = *(pp + 1);
      nfc_ncif_error_status(conn_id, *pp);
      break;

    case NCI_MSG_CORE_CONN_CREDITS:
      nfc_ncif_proc_credits(pp, len);
      break;

    default:
      NFC_TRACE_ERROR1("unknown opcode:0x%x", op_code);
      break;
  }
}

/*******************************************************************************
**
** Function         nci_proc_rf_management_rsp
**
** Description      Process NCI responses in the RF Management group
**
** Returns          void
**
*******************************************************************************/
void nci_proc_rf_management_rsp(NFC_HDR* p_msg) {
  uint8_t* p;
  uint8_t* pp, len, op_code;
  uint8_t* p_old = nfc_cb.last_cmd;

  /* find the start of the NCI message and parse the NCI header */
  p = (uint8_t*)(p_msg + 1) + p_msg->offset;
  pp = p + 1;
  NCI_MSG_PRS_HDR1(pp, op_code);
  len = *pp++;

  switch (op_code) {
    case NCI_MSG_RF_DISCOVER:
      nfa_dm_p2p_prio_logic(op_code, pp, NFA_DM_P2P_PRIO_RSP);
      nfc_ncif_rf_management_status(NFC_START_DEVT, *pp);
      break;

    case NCI_MSG_RF_DISCOVER_SELECT:
      nfc_ncif_rf_management_status(NFC_SELECT_DEVT, *pp);
      break;

    case NCI_MSG_RF_T3T_POLLING:
      break;

    case NCI_MSG_RF_DISCOVER_MAP:
      nfc_ncif_rf_management_status(NFC_MAP_DEVT, *pp);
      break;

    case NCI_MSG_RF_DEACTIVATE:
      if (false == nfa_dm_p2p_prio_logic(op_code, pp, NFA_DM_P2P_PRIO_RSP)) {
        return;
      }
      nfc_ncif_proc_deactivate(*pp, *p_old, false);
      break;

#if (NFC_NFCEE_INCLUDED == true)
#if (NFC_RW_ONLY == FALSE)

    case NCI_MSG_RF_SET_ROUTING:
      nfc_ncif_event_status(NFC_SET_ROUTING_REVT, *pp);
      break;

    case NCI_MSG_RF_GET_ROUTING:
      if (*pp != NFC_STATUS_OK)
        nfc_ncif_event_status(NFC_GET_ROUTING_REVT, *pp);
      break;
#endif
#endif

    case NCI_MSG_RF_PARAMETER_UPDATE:
      nfc_ncif_event_status(NFC_RF_COMM_PARAMS_UPDATE_REVT, *pp);
      break;

    case NCI_MSG_RF_ISO_DEP_NAK_PRESENCE:
      nfc_ncif_proc_isodep_nak_presence_check_status(*pp, false);
      break;
    default:
      NFC_TRACE_ERROR1("unknown opcode:0x%x", op_code);
      break;
  }
}

/*******************************************************************************
**
** Function         nci_proc_rf_management_ntf
**
** Description      Process NCI notifications in the RF Management group
**
** Returns          void
**
*******************************************************************************/
void nci_proc_rf_management_ntf(NFC_HDR* p_msg) {
  uint8_t* p;
  uint8_t* pp, len, op_code;

  /* find the start of the NCI message and parse the NCI header */
  p = (uint8_t*)(p_msg + 1) + p_msg->offset;
  pp = p + 1;
  NCI_MSG_PRS_HDR1(pp, op_code);
  len = *pp++;

  switch (op_code) {
    case NCI_MSG_RF_DISCOVER:
      nfc_ncif_proc_discover_ntf(p, p_msg->len);
      break;

    case NCI_MSG_RF_DEACTIVATE:
      if (false == nfa_dm_p2p_prio_logic(op_code, pp, NFA_DM_P2P_PRIO_NTF)) {
        return;
      }
      nfc_ncif_proc_deactivate(NFC_STATUS_OK, *pp, true);
      break;

    case NCI_MSG_RF_INTF_ACTIVATED:
      if (false == nfa_dm_p2p_prio_logic(op_code, pp, NFA_DM_P2P_PRIO_NTF)) {
        return;
      }
      nfc_ncif_proc_activate(pp, len);
      break;

    case NCI_MSG_RF_FIELD:
      nfc_ncif_proc_rf_field_ntf(*pp);
      break;

    case NCI_MSG_RF_T3T_POLLING:
      nfc_ncif_proc_t3t_polling_ntf(pp, len);
      break;

#if (NFC_NFCEE_INCLUDED == true)
#if (NFC_RW_ONLY == FALSE)

    case NCI_MSG_RF_GET_ROUTING:
      nfc_ncif_proc_get_routing(pp, len);
      break;

    case NCI_MSG_RF_EE_ACTION:
      nfc_ncif_proc_ee_action(pp, len);
      break;

    case NCI_MSG_RF_EE_DISCOVERY_REQ:
      nfc_ncif_proc_ee_discover_req(pp, len);
      break;
#endif
#endif
    case NCI_MSG_RF_ISO_DEP_NAK_PRESENCE:
      nfc_ncif_proc_isodep_nak_presence_check_status(*pp, true);
      break;
    default:
      NFC_TRACE_ERROR1("unknown opcode:0x%x", op_code);
      break;
  }
}

#if (NFC_NFCEE_INCLUDED == true)
#if (NFC_RW_ONLY == FALSE)

/*******************************************************************************
**
** Function         nci_proc_ee_management_rsp
**
** Description      Process NCI responses in the NFCEE Management group
**
** Returns          void
**
*******************************************************************************/
void nci_proc_ee_management_rsp(NFC_HDR* p_msg) {
  uint8_t* p;
  uint8_t* pp, len, op_code;
  tNFC_RESPONSE_CBACK* p_cback = nfc_cb.p_resp_cback;
  tNFC_NFCEE_DISCOVER_REVT nfcee_discover;
  tNFC_NFCEE_MODE_SET_REVT mode_set;
  tNFC_NFCEE_PL_CONTROL_REVT  pl_control;
#if (NXP_EXTNS == TRUE)
  tNFC_NFCEE_MODE_SET_INFO mode_set_info;
  tNFC_NFCEE_EE_PWR_LNK_REVT pwr_lnk_ctrl;

#endif
  void* p_evt = NULL;
  tNFC_RESPONSE_EVT event = NFC_NFCEE_INFO_REVT;
  uint8_t* p_old = nfc_cb.last_cmd;

  /* find the start of the NCI message and parse the NCI header */
  p = (uint8_t*)(p_msg + 1) + p_msg->offset;
  pp = p + 1;
  NCI_MSG_PRS_HDR1(pp, op_code);
  NFC_TRACE_DEBUG1("nci_proc_ee_management_rsp opcode:0x%x", op_code);
  len = *pp++;

  switch (op_code) {
    case NCI_MSG_NFCEE_DISCOVER:
      p_evt = (void*)&nfcee_discover;
      nfcee_discover.status = *pp++;
      nfcee_discover.num_nfcee = *pp++;

      if (nfcee_discover.status != NFC_STATUS_OK) nfcee_discover.num_nfcee = 0;

      event = NFC_NFCEE_DISCOVER_REVT;
      break;

    case NCI_MSG_NFCEE_MODE_SET:
      p_evt = (void*)&mode_set;
      mode_set.status = *pp;
      mode_set.nfcee_id = 0;
      event = NFC_NFCEE_MODE_SET_REVT;
      mode_set.nfcee_id = *p_old++;
      mode_set.mode = *p_old++;
      if (nfc_cb.nci_version != NCI_VERSION_2_0 || *pp != NCI_STATUS_OK)
      {
          nfc_cb.flags  &= ~NFC_FL_WAIT_MODE_SET_NTF;
          event               = NFC_NFCEE_MODE_SET_REVT;
      }
      else
      {
          /* else response reports OK status on notification */
          return;
      }
      break;

#if (NXP_EXTNS == TRUE)
    case NCI_MSG_NFCEE_PWR_LNK_CTRL:
        if(nfcFL.eseFL._WIRED_MODE_STANDBY) {
            p_evt = (tNFC_RESPONSE*)&pwr_lnk_ctrl;
            pwr_lnk_ctrl.status = *pp;
            pwr_lnk_ctrl.nfcee_id = 0;
            event = NFC_NFCEE_PWR_LNK_CTRL_REVT;
            pwr_lnk_ctrl.nfcee_id = *p_old++;
            if(nfcFL.eseFL._ESE_DUAL_MODE_PRIO_SCHEME ==
                    nfcFL.eseFL._ESE_WIRED_MODE_RESUME) {
                if (nfc_cb.bIssueModeSetCmd) {
                    NFC_TRACE_DEBUG0("mode set cmd send after pwrlink cmd");
                    nfc_cb.bSetmodeOnReq = true;
                    nci_snd_nfcee_mode_set(NFCEE_ID_ESE, NFC_MODE_ACTIVATE);
                    nfc_start_timer(&nfc_cb.nci_wait_setMode_Ntf_timer,
                            (uint16_t)NFC_TYPE_NCI_WAIT_SETMODE_NTF,
                            NFC_NCI_SETMODE_NTF_TIMEOUT);
                    nfc_cb.bIssueModeSetCmd = false;
                } else {
                    nfc_cb.bIssueModeSetCmd = false;
                }
            }
            break;
        }
#endif
    case NCI_MSG_NFCEE_POWER_LINK_CTRL:
        p_evt                   = (tNFC_RESPONSE *) &pl_control;
        pl_control.status        = *pp;
        pl_control.nfcee_id      = *p_old++;
        pl_control.pl_control    = *p_old++;
        event               = NFC_NFCEE_PL_CONTROL_REVT;
        break;
    default:
      p_cback = NULL;
      NFC_TRACE_ERROR1("unknown opcode:0x%x", op_code);
      break;
  }

  if (p_cback) (*p_cback)(event, p_evt);
}

/*******************************************************************************
**
** Function         nci_proc_ee_management_ntf
**
** Description      Process NCI notifications in the NFCEE Management group
**
** Returns          void
**
*******************************************************************************/
void nci_proc_ee_management_ntf(NFC_HDR* p_msg) {
  uint8_t* p;
  uint8_t* pp, len, op_code;
  tNFC_RESPONSE_CBACK* p_cback = nfc_cb.p_resp_cback;
  tNFC_NFCEE_INFO_REVT nfcee_info;
  void* p_evt = (void*)&nfcee_info;
  tNFC_RESPONSE_EVT event = NFC_NFCEE_INFO_REVT;
  uint8_t* p_old = nfc_cb.last_cmd;
  uint8_t xx;
  uint8_t yy;
  uint8_t ee_status;
  tNFC_NFCEE_TLV* p_tlv;
#if (NXP_EXTNS == TRUE)
  tNFC_NFCEE_MODE_SET_INFO mode_set_info;
#endif
  tNFC_NFCEE_MODE_SET_REVT    mode_set;
  tNFC_NFCEE_STATUS_REVT      nfcee_status;
  /* find the start of the NCI message and parse the NCI header */
  p = (uint8_t*)(p_msg + 1) + p_msg->offset;
  pp = p + 1;
  NCI_MSG_PRS_HDR1(pp, op_code);
  NFC_TRACE_DEBUG1("nci_proc_ee_management_ntf opcode:0x%x", op_code);
  len = *pp++;

  if (op_code == NCI_MSG_NFCEE_DISCOVER) {
    nfcee_info.nfcee_id = *pp++;
    ee_status = *pp++;

    nfcee_info.ee_status = ee_status;
    yy = *pp;
    nfcee_info.num_interface = *pp++;
    p = pp;

    if (nfcee_info.num_interface > NFC_MAX_EE_INTERFACE)
      nfcee_info.num_interface = NFC_MAX_EE_INTERFACE;

    for (xx = 0; xx < nfcee_info.num_interface; xx++) {
      nfcee_info.ee_interface[xx] = *pp++;
    }

    pp = p + yy;
    nfcee_info.num_tlvs = *pp++;
    NFC_TRACE_DEBUG4("nfcee_id: 0x%x num_interface:0x%x/0x%x, num_tlvs:0x%x",
                     nfcee_info.nfcee_id, nfcee_info.num_interface, yy,
                     nfcee_info.num_tlvs);

    if (nfcee_info.num_tlvs > NFC_MAX_EE_TLVS)
      nfcee_info.num_tlvs = NFC_MAX_EE_TLVS;

    p_tlv = &nfcee_info.ee_tlv[0];

    for (xx = 0; xx < nfcee_info.num_tlvs; xx++, p_tlv++) {
#if (NXP_EXTNS == TRUE)
      if (*pp < 0xA0) {
        p_tlv->tag = *pp++;
      } else {
        p_tlv->tag = *pp++;
        p_tlv->tag = (p_tlv->tag << 8) | *pp++;
      }
#else
      p_tlv->tag = *pp++;
#endif
      p_tlv->len = yy = *pp++;
      NFC_TRACE_DEBUG2("tag:0x%x, len:0x%x", p_tlv->tag, p_tlv->len);
      if (p_tlv->len > NFC_MAX_EE_INFO) p_tlv->len = NFC_MAX_EE_INFO;
      p = pp;
      STREAM_TO_ARRAY(p_tlv->info, pp, p_tlv->len);
      pp = p += yy;
    }
  }
#if (NXP_EXTNS == TRUE)
  else if (op_code == NCI_MSG_NFCEE_MODE_SET) {
      NFC_TRACE_DEBUG2("nci_proc_ee_management_ntf status:0x%x, nfceeid:0x%x",
              *pp, *(pp + 1));
      nfc_stop_timer(&nfc_cb.nci_wait_setMode_Ntf_timer);
      if(nfc_cb.nci_version != NCI_VERSION_2_0)
      {
          p_evt = (tNFC_RESPONSE*)&mode_set_info;
          event = NFC_NFCEE_MODE_SET_INFO;
          ee_status = *pp++;
          mode_set_info.nfcee_id = *pp++;
          mode_set_info.status = ee_status;
      }
      else
      {
          NFC_TRACE_DEBUG2("nci_proc_ee_management_last ntf mode:0x%x, nfceeid:0x%x",
                  mode_set.mode, mode_set.nfcee_id);
          p_evt   = (tNFC_RESPONSE *) &mode_set;
          mode_set.status = *pp;
          /*mode_set.nfcee_id = *p_old++;
      mode_set.mode = *p_old++;*/
          mode_set.nfcee_id = nfa_ee_cb.nfcee_id;
          mode_set.mode = nfa_ee_cb.mode;
          event   = NFC_NFCEE_MODE_SET_REVT;
          nfc_cb.flags  &= ~NFC_FL_WAIT_MODE_SET_NTF;
          nfc_stop_timer(&nfc_cb.nci_setmode_ntf_timer);
      }
      if ((nfcFL.eseFL._ESE_DUAL_MODE_PRIO_SCHEME ==
              nfcFL.eseFL._ESE_WIRED_MODE_RESUME) &&
              ((nfc_cb.bBlockWiredMode) && (nfc_cb.bSetmodeOnReq))) {
          nfc_cb.bSetmodeOnReq = false;
          nfc_cb.bBlockWiredMode = false;
          nfc_cb.bCeActivatedeSE = false;
          nfc_ncif_allow_dwp_transmission();
      }
  }
#endif
  else if (op_code == NCI_MSG_NFCEE_STATUS) {
      p_evt   = (tNFC_RESPONSE *) &nfcee_status;
      event   = NFC_NFCEE_STATUS_REVT;
      nfcee_status.status         = NCI_STATUS_OK;
      nfcee_status.nfcee_id       = *pp++;
      nfcee_status.nfcee_status   = *pp;
     }
  else {
    p_cback = NULL;
    NFC_TRACE_ERROR1("unknown opcode:0x%x", op_code);
  }

  if (p_cback) (*p_cback)(event, p_evt);
}

#endif
#endif

/*******************************************************************************
**
** Function         nci_proc_prop_rsp
**
** Description      Process NCI responses in the Proprietary group
**
** Returns          void
**
*******************************************************************************/
void nci_proc_prop_rsp(NFC_HDR* p_msg) {
  uint8_t* p;
  uint8_t* p_evt;
  uint8_t* pp, len, op_code;
  tNFC_VS_CBACK* p_cback = (tNFC_VS_CBACK*)nfc_cb.p_vsc_cback;

  /* find the start of the NCI message and parse the NCI header */
  p = p_evt = (uint8_t*)(p_msg + 1) + p_msg->offset;
  pp = p + 1;
  NCI_MSG_PRS_HDR1(pp, op_code);
  len = *pp++;

  /*If there's a pending/stored command, restore the associated address of the
   * callback function */
  if (p_cback)
    (*p_cback)((tNFC_VS_EVT)(NCI_RSP_BIT | op_code), p_msg->len, p_evt);
}

#if (NXP_EXTNS == TRUE)
/*******************************************************************************
**
** Function         nci_proc_prop_nxp_rsp
**
** Description      Process NXP NCI responses
**
** Returns          void
**
*******************************************************************************/
void nci_proc_prop_nxp_rsp(NFC_HDR* p_msg) {
  uint8_t* p;
  uint8_t* p_evt;
  uint8_t* pp, len, op_code;
  tNFC_VS_CBACK* p_cback = (tNFC_VS_CBACK*)nfc_cb.p_vsc_cback;

  /* find the start of the NCI message and parse the NCI header */
  p = p_evt = (uint8_t*)(p_msg + 1) + p_msg->offset;
  pp = p + 1;
  NCI_MSG_PRS_HDR1(pp, op_code);
  len = *pp++;

  /*If there's a pending/stored command, restore the associated address of the
   * callback function */
  if (p_cback) {
    (*p_cback)((tNFC_VS_EVT)(NCI_RSP_BIT | op_code), p_msg->len, p_evt);
    nfc_cb.p_vsc_cback = NULL;
  }
  nfc_cb.nxpCbflag = false;
  nfc_ncif_update_window();
}
#endif

/*******************************************************************************
**
** Function         nci_proc_prop_ntf
**
** Description      Process NCI notifications in the Proprietary group
**
** Returns          void
**
*******************************************************************************/
void nci_proc_prop_ntf(NFC_HDR* p_msg) {
  uint8_t* p;
  uint8_t* p_evt;
  uint8_t* pp, len, op_code;
  int i;

  /* find the start of the NCI message and parse the NCI header */
  p = p_evt = (uint8_t*)(p_msg + 1) + p_msg->offset;
  pp = p + 1;
  NCI_MSG_PRS_HDR1(pp, op_code);
  len = *pp++;

#if (NXP_EXTNS == TRUE)
  NFC_TRACE_DEBUG1("nci_proc_prop_ntf:op_code =0x%x", op_code);
  switch (op_code) {
    case NCI_MSG_RF_WTX:
      nfc_ncif_proc_rf_wtx_ntf(p, p_msg->len);
      return;
    default:
      break;
  }
#endif

  for (i = 0; i < NFC_NUM_VS_CBACKS; i++) {
    if (nfc_cb.p_vs_cb[i]) {
      (*nfc_cb.p_vs_cb[i])((tNFC_VS_EVT)(NCI_NTF_BIT | op_code), p_msg->len,
                           p_evt);
    }
  }
}
