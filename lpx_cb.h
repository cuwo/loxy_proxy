#pragma once
#include "includes.h"
#include "lpx_sd.h"
#include "lpx_mt.h"
#include "lpx_dbg.h"
#include "lpx_net.h"
#include "lpx_parse.h"

//new client connected
//we must accept it and add into epoll as HTTP
void LpxCbAccept(SD * sda);

//kill the socket entirely (from this side)
void LpxCbKill(SD * sda);

//write the data into socket (and unlock the other, if required)
void LpxCbWrite(SD * sda);

void LpxCbRead(SD * sda);

void LpxPPKill(SD * sda, unsigned int flags);

void LpxFinWr(SD * sda, LpxConstString * err);

void LpxCbDns(SD * sda);

void LpxParseFinish(SD * sda);

void LpxParseClean(SD * sda);

void LpxParseFinishDns(SD * sda);

void LpxCbParse(SD * sda);

void LpxCbConnect(SD * sda);

void LpxCbSuccess(SD * sda);

void LpxSayEstablish(SD * sda);
