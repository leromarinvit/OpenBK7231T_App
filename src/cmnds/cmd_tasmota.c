
#include "../logging/logging.h"
#include "../new_pins.h"
#include "../obk_config.h"
#include <ctype.h>
#include "cmd_local.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../hal/hal_wifi.h"
#ifdef ENABLE_LITTLEFS
	#include "../littlefs/our_lfs.h"
#endif

#define IP_LOCAL '1'
#define IP_GW    '2'
#define IP_MASK  '3'
#define IP_DNS   '4'



int parsePowerArgument(const char *s) {
	if(!stricmp(s,"ON"))
		return 1;
	if(!stricmp(s,"OFF"))
		return 0;
	return atoi(s);
}

static commandResult_t power(const void *context, const char *cmd, const char *args, int cmdFlags){
	//if (!wal_strnicmp(cmd, "POWER", 5)){
		int channel = 0;
		int iVal = 0;
		int i;
		bool bRelayIndexingStartsWithZero;
		
		bRelayIndexingStartsWithZero  = CHANNEL_HasChannelPinWithRoleOrRole(0, IOR_Relay, IOR_Relay_n);

        ADDLOG_INFO(LOG_FEATURE_CMD, "tasCmnd POWER (%s) received with args %s",cmd,args);

		if (strlen(cmd) > 5) {
			channel = atoi(cmd+5);

			if (LED_IsLEDRunning()) {
				channel = SPECIAL_CHANNEL_LEDPOWER;
			}
			else {
				if (bRelayIndexingStartsWithZero) {
					channel--;
					if (channel < 0)
						channel = 0;
				}
			}
		} else {
			// if new LED driver active
			if(LED_IsLEDRunning()) {
				channel = SPECIAL_CHANNEL_LEDPOWER;
			} else {
				// find first active channel, because some people index with 0 and some with 1
				for(i = 0; i < CHANNEL_MAX; i++) {
					if (h_isChannelRelay(i) || CHANNEL_GetType(i) == ChType_Toggle) {
						channel = i;
						break;
					}
				}
			}
		}
#if 0
		// it seems that my Home Assistant expects RGB etc light bulbs to be turned off entirely
		// with this commands with no arguments, so... no arguments = set all channels?
		else
		{
			CHANNEL_SetAll(iVal, false);
			return 1;
		}
#endif
		if(args == 0 || *args == 0) {
			// this should only check status
		}
		else if (!stricmp(args, "STATUS")) {
			// this should only check status
		} else if(!stricmp(args,"TOGGLE")) {
			CHANNEL_Toggle(channel);
		} else {
			iVal = parsePowerArgument(args);
			CHANNEL_Set(channel, iVal, false);
		}
		return CMD_RES_OK;
	//}
	//return 0;
}

static commandResult_t powerAll(const void *context, const char *cmd, const char *args, int cmdFlags){
	//if (!wal_strnicmp(cmd, "POWERALL", 8)){
		int iVal = 0;

        ADDLOG_INFO(LOG_FEATURE_CMD, "tasCmnd POWERALL (%s) received with args %s",cmd,args);

		iVal = parsePowerArgument(args);

		CHANNEL_SetAll(iVal, false);
		return CMD_RES_OK;
	//}
	//return 0;
}


static commandResult_t color(const void *context, const char *cmd, const char *args, int cmdFlags){
   // if (!wal_strnicmp(cmd, "COLOR", 5)){
        if (args[0] != '#'){
            ADDLOG_ERROR(LOG_FEATURE_CMD, "tasCmnd COLOR expected a # prefixed color, you sent %s",args);
            return 0;
        } else {
            const char *c = args;
            int val = 0;
            int channel = 0;
            ADDLOG_DEBUG(LOG_FEATURE_CMD, "tasCmnd COLOR got %s", args);
            c++;
            while (*c){
                char tmp[3];
                int r;
                int val100;
                tmp[0] = *(c++);
                if (!*c) break;
                tmp[1] = *(c++);
                tmp[2] = '\0';
                r = sscanf(tmp, "%x", &val);
                if (!r) {
                    ADDLOG_ERROR(LOG_FEATURE_CMD, "COLOR no sscanf hex result from %s", tmp);
                    break;
                }
                // if this channel is not PWM, find a PWM channel;
                while ((channel < 32) && (IOR_PWM != CHANNEL_GetRoleForOutputChannel(channel) && IOR_PWM_n != CHANNEL_GetRoleForOutputChannel(channel))) {
                    channel ++;
                }

                if (channel >= 32) {
                    ADDLOG_ERROR(LOG_FEATURE_CMD, "COLOR channel >= 32");
                    break;
                }

                val100 = (val * 100)/255;
            //    ADDLOG_DEBUG(LOG_FEATURE_CMD, "COLOR found chan %d -> val255 %d -> val100 %d (from %s)", channel, val, val100, tmp);
                CHANNEL_Set(channel, val100, 0);
                // move to next channel.
                channel ++;
            }
            if (!(*c)){
              //  ADDLOG_DEBUG(LOG_FEATURE_CMD, "COLOR arg ended");
            }
        }
        return CMD_RES_OK;
  //  }
   // return 0;
}


static commandResult_t cmnd_backlog(const void * context, const char *cmd, const char *args, int cmdFlags){
	const char *subcmd;
	const char *p;
	int count = 0;
    char copy[128];
    char *c;
	if (stricmp(cmd, "backlog")){
		return -1;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CMD, "backlog [%s]", args);

	subcmd = args;
	p = args;
	while (*subcmd){
        c = copy;
		while (*p){
			if (*p == ';'){
				p++;
				break;
			}
            *(c) = *(p++);
            if (c - copy < (sizeof(copy)-1)){
                c++;
            }
		}
		*c = 0;
		count++;
		CMD_ExecuteCommand(copy, cmdFlags);
		subcmd = p;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CMD, "backlog executed %d", count);

	return CMD_RES_OK;
}

// Our wrapper for LFS.
// Returns a buffer created with malloc.
// You must free it later.
byte *LFS_ReadFile(const char *fname) {
#ifdef ENABLE_LITTLEFS
	if (lfs_present()){
		lfs_file_t file;
		int lfsres;
		int len;
		int cnt;
		byte *res, *at;

		cnt = 0;

		memset(&file, 0, sizeof(lfs_file_t));
		lfsres = lfs_file_open(&lfs, &file, fname, LFS_O_RDONLY);

		if (lfsres >= 0) {
			ADDLOG_DEBUG(LOG_FEATURE_CMD, "LFS_ReadFile: openned file %s", fname);
			//lfs_file_seek(&lfs,&file,0,LFS_SEEK_END);
			//len = lfs_file_tell(&lfs,&file);
			//lfs_file_seek(&lfs,&file,0,LFS_SEEK_SET);

			len = lfs_file_size(&lfs,&file);

			lfs_file_seek(&lfs,&file,0,LFS_SEEK_SET);

			res = malloc(len+1);
			at = res;

			if(res == 0) {
				ADDLOG_INFO(LOG_FEATURE_CMD, "LFS_ReadFile: openned file %s but malloc failed for %i", fname, len);
			} else {
#if 0
				char buffer[32];
				while(at - res < len) {
					lfsres = lfs_file_read(&lfs, &file, buffer, sizeof(buffer));
					if(lfsres <= 0)
						break;
					memcpy(at,buffer,lfsres);

					at += lfsres;
				}
#elif 1
				lfsres = lfs_file_read(&lfs, &file, res, len);
#elif 0
				int ofs;
				for(ofs = 0; ofs < len; ofs++) {
					lfsres = lfs_file_read(&lfs, &file, &res[ofs], 1);
					if(lfsres <= 0)
						break;

				}
#else
				while(at - res < len) {
					lfsres = lfs_file_read(&lfs, &file, at, 1);
					if(lfsres <= 0)
						break;
					at++;
				}
#endif
				res[len] = 0;
				ADDLOG_DEBUG(LOG_FEATURE_CMD, "LFS_ReadFile: Loaded %i bytes\n",len);
				//ADDLOG_INFO(LOG_FEATURE_CMD, "LFS_ReadFile: Loaded %s\n",res);
			}
			lfs_file_close(&lfs, &file);
			ADDLOG_DEBUG(LOG_FEATURE_CMD, "LFS_ReadFile: closed file %s", fname);
			return res;
		} else {
			ADDLOG_INFO(LOG_FEATURE_CMD, "LFS_ReadFile: failed to file %s", fname);
		}
	} else {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "LFS_ReadFile: lfs is absent");
	}
#endif
	return 0;
}

static commandResult_t cmnd_lfsexec(const void * context, const char *cmd, const char *args, int cmdFlags){
#ifdef ENABLE_LITTLEFS
	ADDLOG_DEBUG(LOG_FEATURE_CMD, "exec %s", args);
	if (lfs_present()){
		lfs_file_t *file = os_malloc(sizeof(lfs_file_t));
		if (file){
			int lfsres;
			char line[256];
			const char *fname = "autoexec.bat";
		    memset(file, 0, sizeof(lfs_file_t));
			if (args && *args){
				fname = args;
			}
			lfsres = lfs_file_open(&lfs, file, fname, LFS_O_RDONLY);
			if (lfsres >= 0) {
				ADDLOG_DEBUG(LOG_FEATURE_CMD, "openned file %s", fname);
				do {
					char *p = line;
					do {
						lfsres = lfs_file_read(&lfs, file, p, 1);
						if ((lfsres <= 0) || (*p < 0x20) || (p - line) == 255){
							*p = 0;
							break;
						}
						p++;
					} while ((p - line) < 255);
					ADDLOG_DEBUG(LOG_FEATURE_CMD, "line is %s", line);

					if (lfsres >= 0){
						if (*line && (*line != '#')){
							if (!(line[0] == '/' && line[1] == '/')) {
								CMD_ExecuteCommand(line, cmdFlags);
							}
						}
					}
				} while (lfsres > 0);

				lfs_file_close(&lfs, file);
				ADDLOG_DEBUG(LOG_FEATURE_CMD, "closed file %s", fname);
			} else {
				ADDLOG_ERROR(LOG_FEATURE_CMD, "no file %s err %d", fname, lfsres);
			}
			os_free(file);
			file = NULL;
		}
	} else {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "lfs is absent");
	}
#endif
	return CMD_RES_OK;
}

static commandResult_t cmnd_SSID1(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	CFG_SetWiFiSSID(Tokenizer_GetArg(0));
	return CMD_RES_OK;
}
static commandResult_t cmnd_Password1(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	CFG_SetWiFiPass(Tokenizer_GetArg(0));
	return CMD_RES_OK;
}
static char *get_ipaddr(char which, char *buf, size_t len) {
	in_addr_t addr;

	switch (which) {
	case IP_LOCAL:
		addr = CFG_GetLocalIP();
		break;
	case IP_GW:
		addr = CFG_GetGwIP();
		break;
	case IP_MASK:
		addr = htonl(~0 << (sizeof addr * 8 - CFG_GetNetmask()));
		break;
	case IP_DNS:
		addr = CFG_GetDnsIP();
		break;
	default:
		return NULL;
	}

	return inet_ntoa_r(addr, buf, len);
}
static commandResult_t cmnd_IPAddress(const void * context, const char *cmd, const char *args, int cmdFlags) {
	const int cmdlen = sizeof "ipaddress" - 1;
	char which = 'x';
	char ip[IP4ADDR_STRLEN_MAX];
	in_addr_t addr;
	byte mask = 0;

	if (strlen(cmd) > cmdlen) {
		which = cmd[cmdlen];
	} else {
		char mask[IP4ADDR_STRLEN_MAX];
		char gw[IP4ADDR_STRLEN_MAX];
		char dns[IP4ADDR_STRLEN_MAX];

		get_ipaddr(IP_LOCAL, ip, sizeof ip);
		get_ipaddr(IP_GW, gw, sizeof gw);
		get_ipaddr(IP_MASK, mask, sizeof mask);
		get_ipaddr(IP_DNS, dns, sizeof dns);

		ADDLOG_INFO(LOG_FEATURE_CMD, "tasCmnd IPADDRESS: ip=%s (%s) netmask=%s gw=%s dns=%s", ip, HAL_GetMyIPString(), mask, gw, dns);

		return CMD_RES_OK;
	}

	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);

	switch (Tokenizer_GetArgsCount()) {
	case 1:
		if (!inet_aton(Tokenizer_GetArg(0), &addr)) {
			ADDLOG_ERROR(LOG_FEATURE_CMD, "tasCmnd IPADDRESS: bad address");
			return CMD_RES_BAD_ARGUMENT;
		}
		switch (which) {
		case IP_LOCAL:
			CFG_SetLocalIP(addr);
			break;
		case IP_GW:
			CFG_SetGwIP(addr);
			break;
		case IP_MASK:
			addr = ntohl(addr);
			while (addr & (1 << (sizeof addr * 8 - 1))) {
				addr <<= 1;
				mask++;
			}
			if (addr || mask == sizeof addr * 8) {
				ADDLOG_ERROR(LOG_FEATURE_CMD, "tasCmnd IPADDRESS: bad netmask addr=%x mask=%u", addr, mask);
				return CMD_RES_BAD_ARGUMENT;
			}
			CFG_SetNetmask(mask);
			break;
		case IP_DNS:
			CFG_SetDnsIP(addr);
			break;
		default:
			ADDLOG_ERROR(LOG_FEATURE_CMD, "tasCmnd IPADDRESS: no such address: '%c", which);
			return CMD_RES_ERROR;
		}
		// fall-through
	case 0:
		if (NULL == get_ipaddr(which, ip, sizeof ip)) {
			ADDLOG_ERROR(LOG_FEATURE_CMD, "tasCmnd IPADDRESS: no such address: '%c'", which);
			return CMD_RES_ERROR;
		}

		if (which == IP_LOCAL) {
			ADDLOG_INFO(LOG_FEATURE_CMD, "tasCmnd IPADDRESS: ipaddress%c: %s (%s)", which, ip, HAL_GetMyIPString());
		} else {
			ADDLOG_INFO(LOG_FEATURE_CMD, "tasCmnd IPADDRESS: ipaddress%c: %s", which, ip);
		}
		return CMD_RES_OK;
	default:
		ADDLOG_ERROR(LOG_FEATURE_CMD, "tasCmnd IPADDRESS: too many arguments");
		return CMD_RES_BAD_ARGUMENT;
	}

	return CMD_RES_OK;
}
static commandResult_t cmnd_MqttHost(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	CFG_SetMQTTHost(Tokenizer_GetArg(0));
	return CMD_RES_OK;
}
static commandResult_t cmnd_MqttUser(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	CFG_SetMQTTUserName(Tokenizer_GetArg(0));
	return CMD_RES_OK;
}
static commandResult_t cmnd_MqttClient(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	CFG_SetMQTTClientId(Tokenizer_GetArg(0));
	return CMD_RES_OK;
}
static commandResult_t cmnd_State(const void * context, const char *cmd, const char *args, int cmdFlags) {

	return CMD_RES_OK;
}
static commandResult_t cmnd_MqttPassword(const void * context, const char *cmd, const char *args, int cmdFlags) {
	Tokenizer_TokenizeString(args, TOKENIZER_ALLOW_QUOTES);
	// following check must be done after 'Tokenizer_TokenizeString',
	// so we know arguments count in Tokenizer. 'cmd' argument is
	// only for warning display
	if (Tokenizer_CheckArgsCountAndPrintWarning(cmd, 1)) {
		return CMD_RES_NOT_ENOUGH_ARGUMENTS;
	}
	CFG_SetMQTTPass(Tokenizer_GetArg(0));
	return CMD_RES_OK;
}
int taslike_commands_init(){
	//cmddetail:{"name":"power","args":"[OnorOfforToggle]",
	//cmddetail:"descr":"Tasmota-style POWER command. Should work for both LEDs and relay-based devices. You can write POWER0, POWER1, etc to access specific relays.",
	//cmddetail:"fn":"power","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("power", power, NULL);
	//cmddetail:{"name":"powerAll","args":"",
	//cmddetail:"descr":"set all outputs",
	//cmddetail:"fn":"powerAll","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("powerAll", powerAll, NULL);
	//cmddetail:{"name":"color","args":"[HexString]",
	//cmddetail:"descr":"set PWN color using #RRGGBB[cw][ww]. Do not use it. Use led_basecolor_rgb",
	//cmddetail:"fn":"color","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("color", color, NULL);
	//cmddetail:{"name":"backlog","args":"[string of commands separated with ;]",
	//cmddetail:"descr":"run a sequence of ; separated commands",
	//cmddetail:"fn":"cmnd_backlog","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("backlog", cmnd_backlog, NULL);
	//cmddetail:{"name":"exec","args":"[Filename]",
	//cmddetail:"descr":"exec <file> - run autoexec.bat or other file from LFS if present",
	//cmddetail:"fn":"cmnd_lfsexec","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("exec", cmnd_lfsexec, NULL);
	//cmddetail:{"name":"SSID1","args":"[ValueString]",
	//cmddetail:"descr":"Sets the SSID of target WiFi. Command keeps Tasmota syntax.",
	//cmddetail:"fn":"cmnd_SSID1","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("SSID1", cmnd_SSID1, NULL);
	//cmddetail:{"name":"Password1","args":"[ValueString]",
	//cmddetail:"descr":"Sets the Pass of target WiFi. Command keeps Tasmota syntax",
	//cmddetail:"fn":"cmnd_Password1","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("Password1", cmnd_Password1, NULL);
	//cmddetail:{"name":"ipaddressN","args":"[IP address]",
	//cmddetail:"descr":"Sets WiFi IP/gateway/mask/DNS. Command keeps Tasmota syntax",
	//cmddetail:"fn":"cmnd_IPAddress","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("ipaddress", cmnd_IPAddress, NULL);
	//cmddetail:{"name":"MqttHost","args":"[ValueString]",
	//cmddetail:"descr":"Sets the MQTT host. Command keeps Tasmota syntax",
	//cmddetail:"fn":"cmnd_MqttHost","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MqttHost", cmnd_MqttHost, NULL);
	//cmddetail:{"name":"MqttUser","args":"[ValueString]",
	//cmddetail:"descr":"Sets the MQTT user. Command keeps Tasmota syntax",
	//cmddetail:"fn":"cmnd_MqttUser","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MqttUser", cmnd_MqttUser, NULL);
	//cmddetail:{"name":"MqttPassword","args":"[ValueString]",
	//cmddetail:"descr":"Sets the MQTT pass. Command keeps Tasmota syntax",
	//cmddetail:"fn":"cmnd_MqttPassword","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MqttPassword", cmnd_MqttPassword, NULL);
	//cmddetail:{"name":"MqttClient","args":"[ValueString]",
	//cmddetail:"descr":"Sets the MQTT client. Command keeps Tasmota syntax",
	//cmddetail:"fn":"cmnd_MqttClient","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("MqttClient", cmnd_MqttClient, NULL);
	//cmddetail:{"name":"State","args":"NULL",
	//cmddetail:"descr":"NULL",
	//cmddetail:"fn":"cmnd_State","file":"cmnds/cmd_tasmota.c","requires":"",
	//cmddetail:"examples":""}
	CMD_RegisterCommand("State", cmnd_State, NULL);
    return 0;
}
