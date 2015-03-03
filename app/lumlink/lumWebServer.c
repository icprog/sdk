/*
******************************
*Company:Lumlink
*Data:2015-02-26
*Author:Meiyusong
******************************
*/


#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"


#include "lumlink/lumCommon.h"


static const char *staConfig =
    "<html><head><title>ESP8266 Station Config</title></head><body><form action='/staConfig.cg' method='post'>\
		<input type='hidden' id='hi' name='hi' value=''/>\
		Station SSID:<input type='text' name='s_s' value='%s'/><br/>\
		Station Password: <input type='text' name='s_p' value='%s' /><br/>\
		<input type='submit' value='Submit' onclick='document.getElementById(\"hi\").value=\"tj\"'/>%s</form></body></html>";

static const char *apConfig =
    "<html><head><title>ESP8266 AP Config</title></head><body><form action='/apConfig.cg' method='post'>\
		<input type='hidden' id='hi' name='hi' value=''/>\
		Ap SSID:<input type='text' name='a_s' value='%s'/><br/>\
		Ap Password: <input type='text' name='a_p' value='%s' /><br/>\
		Ap Mode: <select name='a_m'><option value='0'>OPEN</option>\
		<option value='1'>WEP</option><option value='2'>WPA_PSK</option>\
		<option value='3'>WPA2_PSK</option><option value='4'>WPA_WPA2_PSK</option></select><br/>\
		Hidden SSID:<input type='checkbox' name='a_h' value='1' /><br/>\
		<input type='submit' value='Submit' onclick='document.getElementById(\"hi\").value=\"tj\"'/>%s</form></body></html>";

static const char *index1 =
    "<html><head><title>ESP8266 Webserver</title></head><body><p><a href='/staConfig.cg'>Station Config</a></p></br></br><p>\
		<a href='/apConfig.cg'>AP Config</a></p></body></html>";

static const char* httphead = "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-type: text/html\r\nPragma: no-cache\r\n\r\n";

static void USER_FUNC lum_htmlSend(void *arg, char *psend)
{
	uint16 head_len = 2;
	uint16 html_len = os_strlen(psend);
	char *pbuf = NULL;

	pbuf = (char *) os_zalloc(os_strlen(httphead) + html_len + 2);
	os_sprintf(pbuf, httphead, html_len);
	struct espconn *ptrespconn = arg;
	head_len += os_strlen(httphead);
	lumDebug("\nhttphead_len:%d, send_len:%d\n", os_strlen(httphead),
	         strlen(psend));
	os_memcpy(pbuf + head_len, psend, html_len);

	lumDebug("\n%s\n", pbuf);
	espconn_sent(ptrespconn, pbuf, head_len + html_len);
	os_free(pbuf);
	pbuf = NULL;
}


static void USER_FUNC lum_webServerRecv(void *arg, char *pusrdata, unsigned short length)
{
	struct station_config sta;
	struct softap_config ap;
	char *htm = NULL;
	int len = 0;
	wifi_station_get_config(&sta);
	wifi_softap_get_config(&ap);

	lumDebug("*******pusrdata=%s\n", pusrdata);
	if (os_strstr(pusrdata, "staConfig.cg"))
	{
		if (os_strstr(pusrdata, "hi=tj"))
		{
			char *data = strstr(pusrdata, "\r\n\r\n");
			data += 4;
			char *data1 = strtok(data, "&");
			while (data1 != NULL)
			{
				if (strstr(data1, "s_s=") && strstr(data1, "s_s=") != "s_s=")
				{
					os_strcpy(sta.ssid, data1 + 4);
				}
				else if (strstr(data1, "s_p=")
				         && strstr(data1, "s_p=") != "s_p=")
				{
					os_strcpy(sta.password, data1 + 4);
				}
				data1 = strtok(NULL, "&");
			}

			wifi_station_set_config(&sta);
			wifi_softap_set_config(&ap);
			len = os_strlen(staConfig) + os_strlen(sta.ssid)
			      + os_strlen(sta.password);
			htm = (char *) os_zalloc(len);
			os_sprintf(htm, staConfig, sta.ssid, sta.password, "OK");
		}
		else
		{
			len = os_strlen(staConfig) + os_strlen(sta.ssid)
			      + os_strlen(sta.password);
			htm = (char *) os_zalloc(len);
			os_sprintf(htm, staConfig, sta.ssid, sta.password, "");
		}
		lum_htmlSend(arg, htm);
	}
	else if (os_strstr(pusrdata, "apConfig.cg"))
	{
		if (os_strstr(pusrdata, "hi=tj"))
		{
			char *data = strstr(pusrdata, "\r\n\r\n");
			data += 4;
			lumDebug("apConfigSubmit: %s\n\n", data);
			char *data1 = strtok(data, "&");
			while (data1 != NULL)
			{
				if (strstr(data1, "a_s=") && strstr(data1, "a_s=") != "a_s=")
				{
					os_strcpy(ap.ssid, data1 + 4);
					ap.ssid_len = os_strlen(ap.ssid);
				}
				else if (strstr(data1, "a_p=")
				         && strstr(data1, "a_p=") != "a_p=")
				{
					os_strcpy(ap.password, data1 + 4);
				}
				else if (strstr(data1, "a_m=")
				         && strstr(data1, "a_m=") != "a_m=")
				{
					ap.authmode = atoi(data1 + 4);
				}
				else if (strstr(data1, "a_h=")
				         && strstr(data1, "a_h=") != "a_h=")
				{
					ap.ssid_hidden = atoi(data1 + 4);
				}
				data1 = strtok(NULL, "&");
			}
			wifi_softap_set_config(&ap);
			len = os_strlen(apConfig) + os_strlen(ap.ssid)
			      + os_strlen(ap.password);
			htm = (char *) os_zalloc(len);
			os_sprintf(htm, apConfig, ap.ssid, ap.password, "OK");
		}
		else
		{
			len = os_strlen(apConfig) + os_strlen(ap.ssid)
			      + os_strlen(ap.password);
			htm = (char *) os_zalloc(len);
			os_sprintf(htm, apConfig, ap.ssid, ap.password, "");
		}
		lum_htmlSend(arg, htm);
	}
	else
	{
		htm = (char *) os_zalloc(os_strlen(index1));
		os_sprintf(htm, index1);
		lum_htmlSend(arg, htm);
	}
}


static void USER_FUNC lum_webServerReconnect(void *arg, sint8 err)
{
	struct espconn *pesp_conn = arg;

	lumDebug("webserver's %d.%d.%d.%d:%d err %d reconnect\n",
	         pesp_conn->proto.tcp->remote_ip[0],
	         pesp_conn->proto.tcp->remote_ip[1],
	         pesp_conn->proto.tcp->remote_ip[2],
	         pesp_conn->proto.tcp->remote_ip[3],
	         pesp_conn->proto.tcp->remote_port, err);
}


static void USER_FUNC lum_webServerDisconnect(void *arg)
{
	struct espconn *pesp_conn = arg;

	lumDebug("webserver's %d.%d.%d.%d:%d disconnect\n",
	         pesp_conn->proto.tcp->remote_ip[0],
	         pesp_conn->proto.tcp->remote_ip[1],
	         pesp_conn->proto.tcp->remote_ip[2],
	         pesp_conn->proto.tcp->remote_ip[3],
	         pesp_conn->proto.tcp->remote_port);
}


static void USER_FUNC lum_webServerListen(void *arg)
{
	struct espconn *pesp_conn = arg;

	espconn_regist_recvcb(pesp_conn, lum_webServerRecv);
	espconn_regist_reconcb(pesp_conn, lum_webServerReconnect);
	espconn_regist_disconcb(pesp_conn, lum_webServerDisconnect);
}


void USER_FUNC lum_webServerInit(U32 port)
{
	static struct espconn esp_conn;
	static esp_tcp esptcp;

	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->local_port = port;
	espconn_regist_connectcb(&esp_conn, lum_webServerListen);
	espconn_accept(&esp_conn);
}



