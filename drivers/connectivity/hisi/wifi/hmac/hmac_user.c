



#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oam_ext_if.h"
#include "dmac_ext_if.h"
#include "hmac_user.h"
#include "hmac_main.h"
#include "hmac_tx_amsdu.h"
#include "hmac_protection.h"
#include "hmac_smps.h"
#include "hmac_ext_if.h"
#include "hmac_config.h"
#include "hmac_mgmt_ap.h"
#include "hmac_chan_mgmt.h"
#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
#include "hmac_proxy_arp.h"
#endif

#ifdef _PRE_WLAN_FEATURE_WAPI
#include "hmac_wapi.h"
#endif

#ifdef _PRE_WLAN_FEATURE_MCAST
#include "hmac_m2u.h"
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM
#include "hmac_roam_main.h"
#endif //_PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_SUPPORT_ACS
#include "hmac_acs.h"
#endif

#include "hmac_blockack.h"
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_USER_C
/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


/*****************************************************************************
  3 函数实现
*****************************************************************************/


hmac_user_stru*  mac_res_get_hmac_user_alloc(oal_uint16 us_idx)
{
    hmac_user_stru *pst_hmac_user;

    pst_hmac_user = (hmac_user_stru*)_mac_res_get_hmac_user(us_idx);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG1(0, OAM_SF_UM, "{mac_res_get_hmac_user_init::pst_hmac_user null,user_idx=%d.}", us_idx);
        return OAL_PTR_NULL;
    }

    /* 重复申请异常,避免影响业务，暂时打印error但正常申请 */
    if (MAC_USER_ALLOCED == pst_hmac_user->st_user_base_info.uc_is_user_alloced)
    {
        OAM_WARNING_LOG1(0, OAM_SF_UM, "{mac_res_get_hmac_user_init::[E]user has been alloced,user_idx=%d.}", us_idx);
    }

    return pst_hmac_user;
}


hmac_user_stru*  mac_res_get_hmac_user(oal_uint16 us_idx)
{
    hmac_user_stru *pst_hmac_user;

    pst_hmac_user = (hmac_user_stru*)_mac_res_get_hmac_user(us_idx);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        //OAM_ERROR_LOG1(0, OAM_SF_UM, "{mac_res_get_hmac_user::pst_hmac_user null,user_idx=%d.}", us_idx);
        return OAL_PTR_NULL;
    }

    /* 异常: 用户资源已被释放, user idx0 为组播user */
    if ((MAC_USER_ALLOCED != pst_hmac_user->st_user_base_info.uc_is_user_alloced) && (0 != us_idx))
    {
        OAM_WARNING_LOG1(0, OAM_SF_UM, "{mac_res_get_hmac_user::[E]user has been freed,user_idx=%d.}", us_idx);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        oal_mem_print_funcname(oal_get_func_return_address());
#endif
        //return OAL_PTR_NULL;
    }

    return pst_hmac_user;
}



oal_uint32  hmac_user_alloc(oal_uint16 *pus_user_idx)
{
    hmac_user_stru *pst_hmac_user;
    oal_uint32      ul_rslt;
    oal_uint16      us_user_idx_temp;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pus_user_idx))
    {
        OAM_ERROR_LOG0(0, OAM_SF_UM, "{hmac_user_alloc::pus_user_idx null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请hmac user内存 */
    /*lint -e413*/
    ul_rslt = mac_res_alloc_hmac_user(&us_user_idx_temp, OAL_OFFSET_OF(hmac_user_stru, st_user_base_info));
    if (OAL_SUCC != ul_rslt)
    {
        OAM_WARNING_LOG1(0, OAM_SF_UM, "{hmac_user_alloc::mac_res_alloc_hmac_user failed[%d].}", ul_rslt);
        return ul_rslt;
    }
    /*lint +e413*/

    pst_hmac_user = mac_res_get_hmac_user_alloc(us_user_idx_temp);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        mac_res_free_mac_user(us_user_idx_temp);
        OAM_ERROR_LOG1(0, OAM_SF_UM, "{hmac_user_alloc::pst_hmac_user null,user_idx=%d.}", us_user_idx_temp);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 初始清0 */
    OAL_MEMZERO(pst_hmac_user, OAL_SIZEOF(hmac_user_stru));
    /* 标记user资源已被alloc */
    pst_hmac_user->st_user_base_info.uc_is_user_alloced = MAC_USER_ALLOCED;

    *pus_user_idx = us_user_idx_temp;

    return OAL_SUCC;
}


oal_uint32  hmac_user_free(oal_uint16 us_idx)
{
    hmac_user_stru *pst_hmac_user;
    oal_uint32      ul_ret;

    pst_hmac_user = mac_res_get_hmac_user(us_idx);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG1(0, OAM_SF_UM, "{hmac_user_free::pst_hmac_user null,user_idx=%d.}", us_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 重复释放异常, 继续释放不返回 */
    if (MAC_USER_FREED == pst_hmac_user->st_user_base_info.uc_is_user_alloced)
    {
        OAM_WARNING_LOG1(0, OAM_SF_UM, "{hmac_user_free::[E]user has been freed,user_idx=%d.}", us_idx);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        oal_mem_print_funcname(oal_get_func_return_address());
#endif
    }

    ul_ret = mac_res_free_mac_user(us_idx);
    if(OAL_SUCC == ul_ret)
    {
        /* 清除alloc标志 */
        pst_hmac_user->st_user_base_info.uc_is_user_alloced = MAC_USER_FREED;
    }

    return ul_ret;
}


oal_uint32  hmac_user_init(hmac_user_stru *pst_hmac_user)
{
    oal_uint8        uc_tid_loop;
    hmac_ba_tx_stru *pst_tx_ba;

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    oal_uint8        uc_ac_idx;
    oal_uint8        uc_data_idx;
#endif

    /* 初始化tid信息 */
    for (uc_tid_loop = 0; uc_tid_loop < WLAN_TID_MAX_NUM; uc_tid_loop++)
    {
        pst_hmac_user->ast_tid_info[uc_tid_loop].uc_tid_no      = (oal_uint8)uc_tid_loop;

        //pst_hmac_user->ast_tid_info[uc_tid_loop].pst_hmac_user  = (oal_void *)pst_hmac_user;
        pst_hmac_user->ast_tid_info[uc_tid_loop].us_hmac_user_idx = pst_hmac_user->st_user_base_info.us_assoc_id;

        /* 初始化ba rx操作句柄 */
        pst_hmac_user->ast_tid_info[uc_tid_loop].pst_ba_rx_info = OAL_PTR_NULL;

        /* 初始化ba tx操作句柄 */
        pst_hmac_user->ast_tid_info[uc_tid_loop].st_ba_tx_info.en_ba_status     = DMAC_BA_INIT;
        pst_hmac_user->ast_tid_info[uc_tid_loop].st_ba_tx_info.uc_addba_attemps = 0;
        pst_hmac_user->ast_tid_info[uc_tid_loop].st_ba_tx_info.uc_dialog_token  = 0;
        pst_hmac_user->ast_tid_info[uc_tid_loop].st_ba_tx_info.en_ba_switch     = OAL_TRUE;
        pst_hmac_user->auc_ba_flag[uc_tid_loop] = 0;

        /* addba req超时处理函数入参填写 */
        pst_tx_ba = &pst_hmac_user->ast_tid_info[uc_tid_loop].st_ba_tx_info;
        pst_tx_ba->st_alarm_data.pst_ba = (oal_void *)pst_tx_ba;
        pst_tx_ba->st_alarm_data.uc_tid = uc_tid_loop;
        pst_tx_ba->st_alarm_data.us_mac_user_idx = pst_hmac_user->st_user_base_info.us_assoc_id;
        pst_tx_ba->st_alarm_data.uc_vap_id = pst_hmac_user->st_user_base_info.uc_vap_id;

        /* 初始化用户关联请求帧参数 */
        pst_hmac_user->puc_assoc_req_ie_buff = OAL_PTR_NULL;
        pst_hmac_user->ul_assoc_req_ie_len   = 0;

    }

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    for (uc_ac_idx = 0; uc_ac_idx < WLAN_WME_AC_BUTT; uc_ac_idx++)
    {
        for (uc_data_idx = 0; uc_data_idx < WLAN_TXRX_DATA_BUTT; uc_data_idx++)
        {
            pst_hmac_user->aaul_txrx_data_stat[uc_ac_idx][uc_data_idx] = 0;
        }
    }
#endif

    pst_hmac_user->pst_defrag_netbuf = OAL_PTR_NULL;
    pst_hmac_user->en_user_bw_limit  = OAL_FALSE;
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    pst_hmac_user->st_sa_query_info.ul_sa_query_count      = 0;
    pst_hmac_user->st_sa_query_info.ul_sa_query_start_time = 0;
#endif
    OAL_MEMZERO(&pst_hmac_user->st_defrag_timer, OAL_SIZEOF(frw_timeout_stru));
    pst_hmac_user->ul_rx_pkt_drop = 0;

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
    /* 清除usr统计信息 */
    oam_stats_clear_user_stat_info(pst_hmac_user->st_user_base_info.us_assoc_id);
#endif

    pst_hmac_user->ul_first_add_time = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    return OAL_SUCC;
}

oal_uint32  hmac_user_set_avail_num_space_stream(mac_user_stru *pst_mac_user, wlan_nss_enum_uint8 en_vap_nss)
{
    mac_user_ht_hdl_stru         *pst_mac_ht_hdl;
    mac_vht_hdl_stru             *pst_mac_vht_hdl;
    mac_vap_stru                 *pst_mac_vap;
    oal_uint8                     uc_avail_num_spatial_stream = 0;
    oal_uint32                    ul_ret = OAL_SUCC;
    mac_user_nss_stru             st_user_nss;

     /* AP(STA)为legacy设备，只支持1根天线，不需要再判断天线个数 */

    /* 获取HT和VHT结构体指针 */
    pst_mac_ht_hdl  = &(pst_mac_user->st_ht_hdl);
    pst_mac_vht_hdl = &(pst_mac_user->st_vht_hdl);

    if (OAL_TRUE == pst_mac_vht_hdl->en_vht_capable)
    {
        if (3 != pst_mac_vht_hdl->st_rx_max_mcs_map.us_max_mcs_4ss)
        {
            uc_avail_num_spatial_stream = WLAN_FOUR_NSS;
        }
        else if (3 != pst_mac_vht_hdl->st_rx_max_mcs_map.us_max_mcs_3ss)
        {
            uc_avail_num_spatial_stream = WLAN_TRIPLE_NSS;
        }
        else if (3 != pst_mac_vht_hdl->st_rx_max_mcs_map.us_max_mcs_2ss)
        {
            uc_avail_num_spatial_stream = WLAN_DOUBLE_NSS;
        }
        else if (3 != pst_mac_vht_hdl->st_rx_max_mcs_map.us_max_mcs_1ss)
        {
            uc_avail_num_spatial_stream = WLAN_SINGLE_NSS;
        }
        else
        {
            MAC_WARNING_LOG(0, "mac_user_set_avail_num_space_stream: get vht nss error");
            OAM_WARNING_LOG0(pst_mac_user->uc_vap_id, OAM_SF_ANY, "{mac_user_set_avail_num_space_stream::invalid vht nss.}");

            ul_ret =  OAL_FAIL;
        }
    }
    else if (OAL_TRUE == pst_mac_ht_hdl->en_ht_capable)
    {
        if (pst_mac_ht_hdl->uc_rx_mcs_bitmask[3] > 0)
        {
            uc_avail_num_spatial_stream = WLAN_FOUR_NSS;
        }
        else if (pst_mac_ht_hdl->uc_rx_mcs_bitmask[2] > 0)
        {
            uc_avail_num_spatial_stream = WLAN_TRIPLE_NSS;
        }
        else if (pst_mac_ht_hdl->uc_rx_mcs_bitmask[1] > 0)
        {
            uc_avail_num_spatial_stream = WLAN_DOUBLE_NSS;
        }
        else if (pst_mac_ht_hdl->uc_rx_mcs_bitmask[0] > 0)
        {
            uc_avail_num_spatial_stream = WLAN_SINGLE_NSS;
        }
        else
        {
            MAC_WARNING_LOG(0, "mac_user_set_avail_num_space_stream: get ht nss error");
            OAM_WARNING_LOG0(pst_mac_user->uc_vap_id, OAM_SF_ANY, "{mac_user_set_avail_num_space_stream::invalid ht nss.}");

            ul_ret =  OAL_FAIL;
        }
    }
    else
    {
        uc_avail_num_spatial_stream = WLAN_SINGLE_NSS;
    }

    /* 赋值给用户结构体变量 */
    mac_user_set_num_spatial_stream(pst_mac_user, uc_avail_num_spatial_stream);
    mac_user_set_avail_num_spatial_stream(pst_mac_user, OAL_MIN(uc_avail_num_spatial_stream, en_vap_nss));

    /* 空间流信息同步dmac */
    pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_user->uc_vap_id);
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG1(pst_mac_user->uc_vap_id, OAM_SF_CFG, "hmac_user_set_avail_num_space_stream::mac vap(idx=%d) is null!", pst_mac_user->uc_vap_id);
        return ul_ret;
    }
    st_user_nss.uc_avail_num_spatial_stream = pst_mac_user->uc_avail_num_spatial_stream;
    st_user_nss.uc_num_spatial_stream       = pst_mac_user->uc_num_spatial_stream;
    st_user_nss.us_user_idx = pst_mac_user->us_assoc_id;

    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_NSS,
                                    OAL_SIZEOF(mac_user_nss_stru),
                                    (oal_uint8 *)(&st_user_nss));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_user->uc_vap_id, OAM_SF_CFG, "{hmac_user_set_avail_num_space_stream::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)

OAL_STATIC oal_void hmac_stop_sa_query_timer(hmac_user_stru *pst_hmac_user)
{
    frw_timeout_stru    *pst_sa_query_interval_timer;

    pst_sa_query_interval_timer = &(pst_hmac_user->st_sa_query_info.st_sa_query_interval_timer);
    if (OAL_FALSE != pst_sa_query_interval_timer->en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(pst_sa_query_interval_timer);
    }

    /* 删除timers的入参存储空间 */
    if (OAL_PTR_NULL != pst_sa_query_interval_timer->p_timeout_arg)
    {
       OAL_MEM_FREE((oal_void *)pst_sa_query_interval_timer->p_timeout_arg, OAL_TRUE);
       pst_sa_query_interval_timer->p_timeout_arg = OAL_PTR_NULL;
    }
}
#endif


#ifdef _PRE_WLAN_FEATURE_WAPI
hmac_wapi_stru * hmac_user_get_wapi_ptr(mac_vap_stru *pst_mac_vap, oal_bool_enum_uint8 en_pairwise, oal_uint16 us_pairwise_idx)

{
    hmac_user_stru                  *pst_hmac_user;
    //oal_uint32                       ul_ret;
    oal_uint16                       us_user_index;

    if (OAL_TRUE == en_pairwise)
    {
        us_user_index = us_pairwise_idx;
    }
    else
    {
        us_user_index = pst_mac_vap->us_multi_user_idx;
    }

    //OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_user_get_wapi_ptr::en_pairwise == %u, usridx==%u.}", en_pairwise, us_user_index);
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_index);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_user_get_wapi_ptr::pst_hmac_user null.}");
        return OAL_PTR_NULL;
    }

    return &pst_hmac_user->st_wapi;
}
#endif

mac_ap_type_enum_uint8 hmac_compability_ap_tpye_identify(oal_uint8 *puc_mac_addr)
{
    if(MAC_IS_GOLDEN_AP(puc_mac_addr))
    {
        return MAC_AP_TYPE_GOLDENAP;
    }
    return MAC_AP_TYPE_NORMAL;
}


oal_uint32  hmac_user_del(mac_vap_stru *pst_mac_vap, hmac_user_stru *pst_hmac_user)
{
    oal_uint16                      us_user_index;
    frw_event_mem_stru             *pst_event_mem;
    frw_event_stru                 *pst_event;
    dmac_ctx_del_user_stru         *pst_del_user_payload;
    hmac_vap_stru                  *pst_hmac_vap;
    mac_device_stru                *pst_mac_device;
    mac_user_stru                  *pst_mac_user;
    oal_uint32                      ul_ret;
#ifdef _PRE_WLAN_FEATURE_WMMAC
    oal_uint8                       uc_ac_index = 0;
#endif
#ifdef _PRE_WLAN_FEATURE_WAPI
    hmac_user_stru                 *pst_hmac_user_multi;
#endif
    mac_ap_type_enum_uint8          en_ap_type = MAC_AP_TYPE_BUTT;

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    oal_int8                        pc_param[18]    = {0};
    oal_int8                        pc_tmp[8]       = {0};
    oal_uint16                      us_len;
#endif

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == pst_hmac_user)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_UM, "{hmac_user_del::param null,%d %d.}", pst_mac_vap, pst_hmac_user);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_user = (mac_user_stru*)(&pst_hmac_user->st_user_base_info);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_user))
    {
        OAM_ERROR_LOG1(0, OAM_SF_UM, "{hmac_user_del::pst_mac_user param null,%d.}", pst_mac_user);
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_del::del user[%d] start,is multi user[%d], user mac:XX:XX:XX:XX:%02X:%02X}",
                                pst_mac_user->us_assoc_id,
                                pst_mac_user->en_is_multi_user,
                                pst_mac_user->auc_user_mac_addr[4],
                                pst_mac_user->auc_user_mac_addr[5]);
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    /*清理arp探测timer*/
    if (OAL_TRUE == pst_hmac_user->st_hmac_user_btcoex.st_hmac_btcoex_arp_req_process.st_delba_opt_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_user->st_hmac_user_btcoex.st_hmac_btcoex_arp_req_process.st_delba_opt_timer));
    }
#endif

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    hmac_chan_update_40M_intol_user(pst_mac_vap, &(pst_hmac_user->st_user_base_info), OAL_FALSE);
#endif
    /*删除user时候，需要更新保护机制*/
    ul_ret = hmac_protection_del_user(pst_mac_vap, &(pst_hmac_user->st_user_base_info));
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_UM, "{hmac_user_del::hmac_protection_del_user[%d]}", ul_ret);
    }

     /* 获取用户对应的索引 */
    us_user_index = pst_hmac_user->st_user_base_info.us_assoc_id;

    /* 删除hmac user 的关联请求帧空间 */
    if (OAL_PTR_NULL != pst_hmac_user->puc_assoc_req_ie_buff)
    {
        OAL_MEM_FREE(pst_hmac_user->puc_assoc_req_ie_buff, OAL_TRUE);
        pst_hmac_user->puc_assoc_req_ie_buff = OAL_PTR_NULL;
        pst_hmac_user->ul_assoc_req_ie_len   = 0;
    }
#ifdef _PRE_WLAN_FEATURE_SMPS
    /* 删除用户，更新SMPS能力 */
    hmac_smps_update_status(pst_mac_vap, &(pst_hmac_user->st_user_base_info), OAL_FALSE);
    mac_user_set_sm_power_save(&pst_hmac_user->st_user_base_info, 0);
#endif

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    hmac_stop_sa_query_timer(pst_hmac_user);
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
    hmac_proxy_remove_mac(pst_mac_vap, pst_hmac_user->st_user_base_info.auc_user_mac_addr);
#endif

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_del::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_del::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef  _PRE_WLAN_FEATURE_VOWIFI
    if (OAL_PTR_NULL != pst_hmac_vap->st_vap_base_info.pst_vowifi_cfg_param)
    {
        if (VOWIFI_LOW_THRES_REPORT == pst_hmac_vap->st_vap_base_info.pst_vowifi_cfg_param->en_vowifi_mode)
        {
            /* 针对漫游和去关联场景,切换vowifi语音状态 */
            hmac_config_vowifi_report((&pst_hmac_vap->st_vap_base_info), 0, OAL_PTR_NULL);
        }
    }
#endif /* _PRE_WLAN_FEATURE_VOWIFI */

#ifdef _PRE_WLAN_FEATURE_WAPI
    hmac_wapi_deinit(&pst_hmac_user->st_wapi);

     /*STA模式下，清组播wapi加密端口*/
    pst_hmac_user_multi = (hmac_user_stru *)mac_res_get_hmac_user(pst_hmac_vap->st_vap_base_info.us_multi_user_idx);
    if (OAL_PTR_NULL == pst_hmac_user_multi)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_user_del::mac_res_get_hmac_user fail! user_idx[%u]}",pst_hmac_vap->st_vap_base_info.us_multi_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_wapi_reset_port(&pst_hmac_user_multi->st_wapi);

    pst_mac_device->uc_wapi = OAL_FALSE;

#endif

#ifdef _PRE_WLAN_FEATURE_MCAST
    /*用户去关联时清空snoop链表中的该成员 */
    if (OAL_PTR_NULL != pst_hmac_vap->pst_m2u)
    {
        hmac_m2u_cleanup_snoopwds_node(pst_hmac_user);
    }
#endif

    if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
    {

#ifdef _PRE_WLAN_FEATURE_STA_PM
        mac_vap_set_aid(pst_mac_vap, 0);
#endif
#ifdef _PRE_WLAN_FEATURE_ROAM
        hmac_roam_exit(pst_hmac_vap);
#endif //_PRE_WLAN_FEATURE_ROAM

        en_ap_type = hmac_compability_ap_tpye_identify(pst_mac_user->auc_user_mac_addr);
    }

    /***************************************************************************
        抛事件到DMAC层, 删除dmac用户
    ***************************************************************************/
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_ctx_del_user_stru));
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_del::pst_event_mem null.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_del_user_payload = (dmac_ctx_del_user_stru *)pst_event->auc_event_data;
    pst_del_user_payload->us_user_idx = us_user_index;
    pst_del_user_payload->en_ap_type  = en_ap_type;
#if (_PRE_OS_VERSION_WIN32 != _PRE_OS_VERSION)
    /* TBD: 添加此操作51DMT异常，暂看不出异常原因 */
    /* 用户 mac地址和idx 需至少一份有效，供dmac侧查找待删除的用户 */
    oal_memcmp(pst_del_user_payload->auc_user_mac_addr, pst_mac_user->auc_user_mac_addr, WLAN_MAC_ADDR_LEN);
#endif

    /* 填充事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                        FRW_EVENT_TYPE_WLAN_CTX,
                        DMAC_WLAN_CTX_EVENT_SUB_TYPE_DEL_USER,
                        OAL_SIZEOF(dmac_ctx_del_user_stru),
                        FRW_EVENT_PIPELINE_STAGE_1,
                        pst_mac_vap->uc_chip_id,
                        pst_mac_vap->uc_device_id,
                        pst_mac_vap->uc_vap_id);

    frw_event_dispatch_event(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    hmac_tid_clear(pst_mac_vap, pst_hmac_user);

    if (pst_hmac_user->st_mgmt_timer.en_is_registerd == OAL_TRUE)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_hmac_user->st_mgmt_timer);
    }

    if (pst_hmac_user->st_defrag_timer.en_is_registerd == OAL_TRUE)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_hmac_user->st_defrag_timer);
    }
#ifdef _PRE_WLAN_FEATURE_WMMAC
#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
    /*删除user时删除发送addts req超时定时器*/
    for(uc_ac_index = 0; uc_ac_index < WLAN_WME_AC_BUTT; uc_ac_index++)
    {
        if (pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_index].st_addts_timer.en_is_registerd == OAL_TRUE)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_index].st_addts_timer));
        }

        OAL_MEMZERO(&(pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_index]), OAL_SIZEOF(mac_ts_stru));
        pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_index].uc_tsid = 0xFF;
    }
#endif
#endif

    /* 从vap中删除用户 */
    mac_vap_del_user(pst_mac_vap, us_user_index);
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    if (pst_mac_vap->us_user_nums == 5)
    {
        /* AP用户达到5时，调整流控参数为配置文件原有值 */
        oal_itoa(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_USED_MEM_FOR_START), pc_param, 8);
        oal_itoa(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_USED_MEM_FOR_STOP), pc_tmp, 8);
        pc_param[OAL_STRLEN(pc_param)] = ' ';
        oal_memcmp(pc_param + OAL_STRLEN(pc_param), pc_tmp, OAL_STRLEN(pc_tmp));

        us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);
        hmac_config_sdio_flowctrl(pst_mac_vap, us_len, pc_param);
    }
#endif
    /* 释放用户内存 */
    ul_ret = hmac_user_free(us_user_index);
    if(OAL_SUCC == ul_ret)
    {
        /* device下已关联user个数-- */
        pst_mac_device->uc_asoc_user_cnt--;
    }
    else
    {
        OAM_ERROR_LOG1(0, OAM_SF_UM, "{hmac_user_del::mac_res_free_mac_user fail[%d].}", ul_ret);
    }

#ifdef _PRE_SUPPORT_ACS
    hmac_acs_register_rescan_timer(pst_mac_vap->uc_device_id);
#endif

    return OAL_SUCC;
}


oal_uint32  hmac_user_add(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr, oal_uint16 *pus_user_index)
{
    hmac_vap_stru                  *pst_hmac_vap;
    hmac_user_stru                 *pst_hmac_user;
    oal_uint32                      ul_ret;
    frw_event_mem_stru             *pst_event_mem;
    frw_event_stru                 *pst_event;
    dmac_ctx_add_user_stru         *pst_add_user_payload;
    oal_uint16                      us_user_idx;
    mac_device_stru                *pst_mac_device;
    mac_cfg_80211_ucast_switch_stru st_80211_ucast_switch;
    mac_ap_type_enum_uint8          en_ap_type = MAC_AP_TYPE_BUTT;
#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    oal_uint8                       uc_hipriv_ack = OAL_FALSE;
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    oal_int8                        pc_param[]    = "30 25";
    oal_uint16                      us_len;
#endif

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_mac_addr) || (OAL_PTR_NULL == pus_user_index)))
    {
        OAM_ERROR_LOG3(0, OAM_SF_UM, "{hmac_user_add::param null, %d %d %d.}", pst_mac_vap, puc_mac_addr, pus_user_index);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_UM, "{hmac_user_add::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 在HMAC处作最大活跃用户数判断，已达32则返回错误 */
    if (pst_mac_device->uc_active_user_cnt >= WLAN_ACTIVE_USER_MAX_NUM)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::invalid uc_active_user_cnt[%d].}", pst_mac_device->uc_active_user_cnt);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    if (pst_mac_device->uc_asoc_user_cnt >= pst_mac_device->us_user_nums_max)
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::users nums exceed spec,invalid uc_asoc_user_cnt[%d], us_user_nums_max=%d.}",
                         pst_mac_device->uc_asoc_user_cnt , pst_mac_device->us_user_nums_max);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

#endif

    if (pst_hmac_vap->st_vap_base_info.us_user_nums >= pst_hmac_vap->us_user_nums_max)
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::invalid us_user_nums[%d], us_user_nums_max[%d].}",
                         pst_hmac_vap->st_vap_base_info.us_user_nums, pst_hmac_vap->us_user_nums_max);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    /* 如果此用户已经创建，则返回失败 */
    ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap, puc_mac_addr, &us_user_idx);
    if (OAL_SUCC == ul_ret)
    {
#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
        uc_hipriv_ack = OAL_TRUE;
        hmac_hipriv_proc_write_process_rsp(pst_mac_vap, sizeof(oal_uint8), &uc_hipriv_ack);
#endif
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::mac_vap_find_user_by_macaddr failed[%d].}", ul_ret);
        return OAL_FAIL;
    }

    if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
    {
    #ifdef _PRE_WLAN_FEATURE_P2P
        if (IS_P2P_CL(pst_mac_vap))
        {
            if (pst_hmac_vap->st_vap_base_info.us_user_nums >= 2)
            {
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::a STA can only associated with 2 ap.}");
                return OAL_FAIL;
            }
        }
        else
    #endif
        {
            if (pst_hmac_vap->st_vap_base_info.us_user_nums >= 1)
            {
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::a STA can only associated with one ap.}");
                return OAL_FAIL;
            }
            en_ap_type = hmac_compability_ap_tpye_identify(puc_mac_addr);
        }
#ifdef _PRE_WLAN_FEATURE_ROAM
        hmac_roam_init(pst_hmac_vap);
#endif //_PRE_WLAN_FEATURE_ROAM
    }

    /* 申请hmac用户内存，并初始清0 */
    ul_ret = hmac_user_alloc(&us_user_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::hmac_user_alloc failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* 单播用户不能使用userid为0，需重新申请一个。将userid作为aid分配给对端，处理psm时会出错 */
    if (0 == us_user_idx)
    {
        hmac_user_free(us_user_idx);
        ul_ret = hmac_user_alloc(&us_user_idx);
        if ((OAL_SUCC != ul_ret) || (0 == us_user_idx))
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::hmac_user_alloc failed ret[%d] us_user_idx[%d].}", ul_ret, us_user_idx);
            return ul_ret;
        }
    }

    *pus_user_index = us_user_idx;  /* 出参赋值 */

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_idx);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 初始化mac_user_stru */
    mac_user_init(&(pst_hmac_user->st_user_base_info), us_user_idx, puc_mac_addr,
                  pst_mac_vap->uc_chip_id,
                  pst_mac_vap->uc_device_id,
                  pst_mac_vap->uc_vap_id);

#ifdef _PRE_WLAN_FEATURE_WAPI
    /* 初始化单播wapi对象 */
    hmac_wapi_init(&pst_hmac_user->st_wapi, OAL_TRUE);
    pst_mac_device->uc_wapi = OAL_FALSE;
#endif
    /* 设置amsdu域 */
    hmac_amsdu_init_user(pst_hmac_user);

    /***************************************************************************
        抛事件到DMAC层, 创建dmac用户
    ***************************************************************************/
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_ctx_add_user_stru));
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        /* 异常处理，释放内存，device下关联用户数还没有++，这里不需要判断返回值做--操作 */
        hmac_user_free(us_user_idx);

        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::pst_event_mem null.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_add_user_payload = (dmac_ctx_add_user_stru *)pst_event->auc_event_data;
    pst_add_user_payload->us_user_idx = us_user_idx;
    pst_add_user_payload->en_ap_type  = en_ap_type;
    oal_set_mac_addr(pst_add_user_payload->auc_user_mac_addr, puc_mac_addr);

    /* 填充事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                        FRW_EVENT_TYPE_WLAN_CTX,
                        DMAC_WLAN_CTX_EVENT_SUB_TYPE_ADD_USER,
                        OAL_SIZEOF(dmac_ctx_add_user_stru),
                        FRW_EVENT_PIPELINE_STAGE_1,
                        pst_mac_vap->uc_chip_id,
                        pst_mac_vap->uc_device_id,
                        pst_mac_vap->uc_vap_id);

    ul_ret = frw_event_dispatch_event(pst_event_mem);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        /* 异常处理，释放内存，device下关联用户数还没有++，这里不需要判断返回值做--操作 */
        hmac_user_free(us_user_idx);
        FRW_EVENT_FREE(pst_event_mem);

        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::frw_event_dispatch_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    FRW_EVENT_FREE(pst_event_mem);

    /* 添加用户到MAC VAP */
    ul_ret = mac_vap_add_assoc_user(pst_mac_vap, us_user_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::mac_vap_add_assoc_user failed[%d].}", ul_ret);

        /* 异常处理，释放内存，device下关联用户数还没有++，这里不需要判断返回值做--操作 */
        hmac_user_free(us_user_idx);
        return OAL_FAIL;
    }

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    if (pst_mac_vap->us_user_nums == 6)
    {
        /* AP用户达到6时，调整流控参数为Stop为25，Start为30 */
        us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);
        hmac_config_sdio_flowctrl(pst_mac_vap, us_len, pc_param);
    }
#endif

    /* 初始话hmac user部分信息 */
    hmac_user_init(pst_hmac_user);

    pst_mac_device->uc_asoc_user_cnt++;

    /* 打开80211单播管理帧开关，观察关联过程，关联成功了就关闭 */
    st_80211_ucast_switch.en_frame_direction = OAM_OTA_FRAME_DIRECTION_TYPE_TX;
    st_80211_ucast_switch.en_frame_type = OAM_USER_TRACK_FRAME_TYPE_MGMT;
    st_80211_ucast_switch.en_frame_switch = OAL_SWITCH_ON;
    st_80211_ucast_switch.en_cb_switch = OAL_SWITCH_ON;
    st_80211_ucast_switch.en_dscr_switch = OAL_SWITCH_ON;
    oal_memcmp(st_80211_ucast_switch.auc_user_macaddr,
                (const oal_void *)puc_mac_addr,
                OAL_SIZEOF(st_80211_ucast_switch.auc_user_macaddr));
    hmac_config_80211_ucast_switch(pst_mac_vap,OAL_SIZEOF(st_80211_ucast_switch),(oal_uint8 *)&st_80211_ucast_switch);

    st_80211_ucast_switch.en_frame_direction = OAM_OTA_FRAME_DIRECTION_TYPE_RX;
    st_80211_ucast_switch.en_frame_type = OAM_USER_TRACK_FRAME_TYPE_MGMT;
    st_80211_ucast_switch.en_frame_switch = OAL_SWITCH_ON;
    st_80211_ucast_switch.en_cb_switch = OAL_SWITCH_ON;
    st_80211_ucast_switch.en_dscr_switch = OAL_SWITCH_ON;
    hmac_config_80211_ucast_switch(pst_mac_vap,OAL_SIZEOF(st_80211_ucast_switch),(oal_uint8 *)&st_80211_ucast_switch);



    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add::user[%d] mac:%02X:XX:XX:XX:%02X:%02X}",
                                us_user_idx,
                                puc_mac_addr[0],
                                puc_mac_addr[4],
                                puc_mac_addr[5]);

    return OAL_SUCC;
}


oal_uint32  hmac_config_add_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_add_user_param_stru    *pst_add_user;
    oal_uint16                      us_user_index;
    hmac_vap_stru                  *pst_hmac_vap;
    hmac_user_stru                 *pst_hmac_user;
    oal_uint32                      ul_ret;
    mac_user_ht_hdl_stru            st_ht_hdl;
    oal_uint32                      ul_rslt;
    mac_device_stru                *pst_mac_device;

    pst_add_user = (mac_cfg_add_user_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_UM, "{hmac_config_add_user::pst_hmac_vap null.}");
        return OAL_FAIL;
    }

    ul_ret = hmac_user_add(pst_mac_vap, pst_add_user->auc_mac_addr, &us_user_index);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_add_user::hmac_user_add failed.}", ul_ret);
        return ul_ret;
    }

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_index);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_add_user::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* TBD hmac_config_add_user 此接口删除，相应调用需要整改，duankaiyong&guyanjie */

    /* 设置qos域，后续如有需要可以通过配置命令参数配置 */
    mac_user_set_qos(&pst_hmac_user->st_user_base_info, OAL_TRUE);

    /* 设置HT域 */
    mac_user_get_ht_hdl(&pst_hmac_user->st_user_base_info, &st_ht_hdl);
    st_ht_hdl.en_ht_capable = pst_add_user->en_ht_cap;

    if (OAL_TRUE == pst_add_user->en_ht_cap)
    {
        pst_hmac_user->st_user_base_info.en_cur_protocol_mode                = WLAN_HT_MODE;
        pst_hmac_user->st_user_base_info.en_avail_protocol_mode              = WLAN_HT_MODE;
    }

    /* 设置HT相关的信息:应该在关联的时候赋值 这个值配置的合理性有待考究 2012->page:786 */
    st_ht_hdl.uc_min_mpdu_start_spacing = 6;
    st_ht_hdl.uc_max_rx_ampdu_factor    = 3;
    mac_user_set_ht_hdl(&pst_hmac_user->st_user_base_info, &st_ht_hdl);

    mac_user_set_asoc_state(&pst_hmac_user->st_user_base_info, MAC_USER_STATE_ASSOC);

    /* 设置amsdu域 */
    hmac_amsdu_init_user(pst_hmac_user);

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    /* 重新设置DMAC需要的参数 */
    pst_add_user->us_user_idx = us_user_index;

    ul_ret = hmac_config_send_event(&pst_hmac_vap->st_vap_base_info,
                                    WLAN_CFGID_ADD_USER,
                                    us_len,
                                    puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        /* 异常处理，释放内存 */
        ul_rslt = hmac_user_free(us_user_index);
        if(OAL_SUCC == ul_rslt)
        {
            pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
            if (OAL_PTR_NULL == pst_mac_device)
            {
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_add_user::pst_mac_device null.}");
                return OAL_ERR_CODE_PTR_NULL;
            }

            /* hmac_add_user成功时device下关联用户数已经++, 这里的device下已关联user个数要-- */
            pst_mac_device->uc_asoc_user_cnt--;
        }

        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_add_user::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* 打桩添加用户信息不全，不需要通知算法 */
    //hmac_user_add_notify_alg(&pst_hmac_vap->st_vap_base_info, us_user_index);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if (IS_LEGACY_VAP(pst_mac_vap))
    {
        mac_vap_state_change(pst_mac_vap, MAC_VAP_STATE_UP);
    }
#endif

    return OAL_SUCC;
}


oal_uint32  hmac_config_del_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_del_user_param_stru    *pst_del_user;
    hmac_user_stru                 *pst_hmac_user;
    hmac_vap_stru                  *pst_hmac_vap;
    oal_uint16                      us_user_index;
    oal_uint32                      ul_ret = 0;

    pst_del_user = (mac_cfg_add_user_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_del_user::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取用户对应的索引 */
    ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap, pst_del_user->auc_mac_addr, &us_user_index);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_del_user::mac_vap_find_user_by_macaddr failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* 获取hmac用户 */
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_index);

    ul_ret = hmac_user_del(pst_mac_vap, pst_hmac_user);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_del_user::hmac_user_del failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* 重新设置DMAC需要的参数 */
    pst_del_user->us_user_idx = us_user_index;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event(&pst_hmac_vap->st_vap_base_info,
                                    WLAN_CFGID_DEL_USER,
                                    us_len,
                                    puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        /* 异常处理，释放内存 */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_del_user::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_user_add_multi_user(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_user_index)
{
    oal_uint32      ul_ret;
    oal_uint16      us_user_index;
    mac_user_stru  *pst_mac_user;
#ifdef _PRE_WLAN_FEATURE_WAPI
    hmac_user_stru *pst_hmac_user;
#endif

    ul_ret = hmac_user_alloc(&us_user_index);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add_multi_user::hmac_user_alloc failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* 初始化组播用户基本信息 */
    pst_mac_user = (mac_user_stru *)mac_res_get_mac_user(us_user_index);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_user_add_multi_user::pst_mac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    mac_user_init(pst_mac_user, us_user_index, OAL_PTR_NULL, pst_mac_vap->uc_chip_id,  pst_mac_vap->uc_device_id, pst_mac_vap->uc_vap_id);

    *pus_user_index = us_user_index;

#ifdef _PRE_WLAN_FEATURE_WAPI
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_index);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_user_add_multi_user::get hmac_user fail.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 初始化wapi对象 */
    hmac_wapi_init(&pst_hmac_user->st_wapi, OAL_FALSE);
#endif

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_user_add_multi_user, user index[%d].}", us_user_index);

    return OAL_SUCC;
}




oal_uint32  hmac_user_del_multi_user(mac_vap_stru *pst_mac_vap)
{
#ifdef _PRE_WLAN_FEATURE_WAPI
    hmac_user_stru      *pst_hmac_user;
#endif

#ifdef _PRE_WLAN_FEATURE_WAPI
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(pst_mac_vap->us_multi_user_idx);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_user_add_multi_user::get hmac_user fail.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_wapi_deinit(&pst_hmac_user->st_wapi);
#endif

    hmac_user_free(pst_mac_vap->us_multi_user_idx);

    return OAL_SUCC;
}


#ifdef _PRE_WLAN_FEATURE_WAPI
oal_uint8  hmac_user_is_wapi_connected(oal_uint8 uc_device_id)
{
    oal_uint8               uc_vap_idx;
    hmac_user_stru         *pst_hmac_user_multi;
    mac_device_stru        *pst_mac_device;
    mac_vap_stru           *pst_mac_vap;

    pst_mac_device = mac_res_get_dev(uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG1(0, OAM_SF_UM, "{hmac_user_is_wapi_connected::pst_mac_device null.id %u}", uc_device_id);
        return OAL_FALSE;
    }

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if (!IS_STA(pst_mac_vap))
        {
            continue;
        }

        pst_hmac_user_multi = (hmac_user_stru *)mac_res_get_hmac_user(pst_mac_vap->us_multi_user_idx);
        if ((OAL_PTR_NULL != pst_hmac_user_multi)
    		&& (OAL_TRUE == pst_hmac_user_multi->st_wapi.uc_port_valid))
        {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}
#endif/* #ifdef _PRE_WLAN_FEATURE_WAPI */



oal_uint32  hmac_user_add_notify_alg(mac_vap_stru *pst_mac_vap, oal_uint16 us_user_idx)
{
    frw_event_mem_stru             *pst_event_mem;
    frw_event_stru                 *pst_event;
    dmac_ctx_add_user_stru         *pst_add_user_payload;
    oal_uint32                      ul_ret;
    hmac_user_stru                 *pst_hmac_user;

    /* 抛事件给Dmac，在dmac层挂用户算法钩子 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_ctx_add_user_stru));
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_user_add_notify_alg::pst_event_mem null.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_add_user_payload = (dmac_ctx_add_user_stru *)pst_event->auc_event_data;
    //oal_memcmp(pst_add_user_payload->auc_bssid, pst_mac_vap->auc_bssid, WLAN_MAC_ADDR_LEN);
    pst_add_user_payload->us_user_idx = us_user_idx;
    pst_add_user_payload->us_sta_aid = pst_mac_vap->us_sta_aid;
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_idx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_user))
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hmac_user_add_notify_alg::null param,pst_hmac_user[%d].}",us_user_idx);
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_ERR_CODE_PTR_NULL;
    }

    mac_user_get_vht_hdl(&pst_hmac_user->st_user_base_info, &pst_add_user_payload->st_vht_hdl);
    mac_user_get_ht_hdl(&pst_hmac_user->st_user_base_info, &pst_add_user_payload->st_ht_hdl);
    /* 填充事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                        FRW_EVENT_TYPE_WLAN_CTX,
                        DMAC_WLAN_CTX_EVENT_SUB_TYPE_NOTIFY_ALG_ADD_USER,
                        OAL_SIZEOF(dmac_ctx_add_user_stru),
                        FRW_EVENT_PIPELINE_STAGE_1,
                        pst_mac_vap->uc_chip_id,
                        pst_mac_vap->uc_device_id,
                        pst_mac_vap->uc_vap_id);

    ul_ret = frw_event_dispatch_event(pst_event_mem);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        /* 异常处理，释放内存 */
        FRW_EVENT_FREE(pst_event_mem);

        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_user_add_notify_alg::frw_event_dispatch_event failed[%d].}", ul_ret);
        return ul_ret;
    }
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


hmac_user_stru  *mac_vap_get_hmac_user_by_addr(mac_vap_stru *pst_mac_vap, oal_uint8  *puc_mac_addr)
{
    oal_uint32              ul_ret;
    oal_uint16              us_user_idx   = 0xffff;
    hmac_user_stru         *pst_hmac_user = OAL_PTR_NULL;

    /*根据mac addr找sta索引*/
    ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap, puc_mac_addr, &us_user_idx);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{mac_vap_get_hmac_user_by_addr::find_user_by_macaddr failed[%d].}", ul_ret);
        if (OAL_PTR_NULL != puc_mac_addr)
        {
            OAM_WARNING_LOG3(0, OAM_SF_ANY,"{mac_vap_get_hmac_user_by_addr:: mac_addr[%02x XX XX XX %02x %02x]!.}",
                                puc_mac_addr[0], puc_mac_addr[4], puc_mac_addr[5]);
        }
        return OAL_PTR_NULL;
    }

    /*根据sta索引找到user内存区域*/
    pst_hmac_user = mac_res_get_hmac_user(us_user_idx);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_vap_get_hmac_user_by_addr::user ptr null.}");
    }
    return pst_hmac_user;
}

/*lint -e19*/
oal_module_symbol(hmac_user_alloc);
oal_module_symbol(hmac_user_init);
oal_module_symbol(hmac_config_kick_user);
oal_module_symbol(mac_vap_get_hmac_user_by_addr);
oal_module_symbol(mac_res_get_hmac_user);
/*lint +e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

