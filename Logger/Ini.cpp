﻿// Ini.cpp: implementation of the CIni class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Ini.h"

const char* AppPath(char* tcPath, const char* fName)
{
	if(!tcPath) 
		return tcPath;
	tcPath[0] = 0;
	int nLen = GetModuleFileNameA( NULL, tcPath, MAX_PATH );
	while(nLen)
	{
		if(tcPath[nLen] == '\\')
		{
			if(fName)
				strcpy(tcPath+nLen+1, fName);
			else
				tcPath[nLen]=0;
			break;
		}
		nLen--;
	}
	if(!nLen && fName)
		strcpy(tcPath, fName);
	return tcPath;
}

std::string AppPath(const char* fName)
{
	char tcPath[MAX_PATH] = {0};
	AppPath(tcPath, fName);
	return tcPath;
}

std::wstring ANSIToUtf16(const std::string& str)
{
    int uniocedeLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
    wchar_t* pUnicode = new wchar_t[uniocedeLen + 1];
    memset(pUnicode, 0, (uniocedeLen + 1)*sizeof(wchar_t));
    ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, (LPWSTR)pUnicode, uniocedeLen);
    std::wstring rt = (wchar_t*)pUnicode;
    delete pUnicode;
    return rt;
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIni::CIni(const char* path)
{
	cfgpath[0] = 0;
	if(path) strcpy(cfgpath, path);
	if(!strchr(cfgpath,'\\')|| strchr(cfgpath,'/'))
		AppPath(cfgpath, path);
}

CIni::~CIni()
{

}

/**
@brief Int配置项获取

@param sec Sec名
@param key 键名
@param def 缺省值
@return Int值
*/
int CIni::GetInt(const char* sec, const char* key, int def)
{
	char ret[20] = {0};
	GetPrivateProfileStringA(sec, key, "", ret, sizeof ret, cfgpath);
	if(!ret[0])
	{//返回为空字符串
		_snprintf(ret, sizeof(ret), "%d", def);
		WritePrivateProfileStringA(sec, key, ret, cfgpath);
		return def;
	}
	return atoi(ret);
}

/**
@brief Int配置项获取

@param sec Sec名
@param key 键名
@param value 值
@return Int值
*/
bool CIni::SetInt(const char* sec,const char* key,int value)
{
	char szValue[20]={0};
	_snprintf(szValue, 20, "%d", value);
	return WritePrivateProfileStringA(sec, key, szValue, cfgpath);
}

/**
@brief Str配置项获取

@param sec Sec名
@param key 键名
@param def 缺省值
@return Str值
*/
std::string CIni::GetStr(const char* sec, const char* key, const char* def)
{
	char ret[MAX_PATH] = {0};
	GetPrivateProfileStringA(sec, key, "", ret, sizeof ret, cfgpath);
	if(ret[0])
		return ret;
	else if(def && def[0])
		WritePrivateProfileStringA(sec, key, def, cfgpath);
	return def;
}

/**
@brief Str配置项设置

@param sec Sec名
@param key 键名
@param value 缺省值
@return Str值
*/
bool CIni::SetStr(const char* sec, const char* key, const char* value)
{
	return WritePrivateProfileStringA(sec, key, value, cfgpath);
}

static CIni gIni("config.ini");
int GetIniInt(const char* sec, const char* key, int def)
{
	return gIni.GetInt(sec,key, def);
}

std::string GetIniStr(const char* sec, const char* key, const char* def)
{
	return gIni.GetStr(sec,key,def);
}
std::wstring GetIniStrW(const char* sec, const char* key, const char* def)
{
    return gIni.GetStrW(sec, key, def);
}
bool SetIniInt(const char* sec, const char* key, int value)
{
	return gIni.SetInt(sec, key, value);
}

bool SetIniStr(const char* sec, const char* key, const char* value)
{
	return gIni.SetStr(sec, key, value);
}