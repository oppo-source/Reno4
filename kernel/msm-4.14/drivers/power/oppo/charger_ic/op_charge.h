/**********************************************************************************
* Copyright (c)  2008-2019  Guangdong OPPO Mobile Comm Corp., Ltd
* VENDOR_EDIT
* Description: OPPO Charge Module Device Tree
* 
* Version: 1.0
* Date: 2019-06-10
* ------------------------------ Revision History: --------------------------------
* <version>           <date>                <author>                            <desc>
*                   2019-07-03            Yichun.Chen                     add common function
***********************************************************************************/
#ifndef __OP_CHARGE_H__
#define __OP_CHARGE_H__

struct ntc_table {
	int resistance;		/* ohm */
	int temperature;	/* 0.1*celsius */
};

struct usb_temp {
	int temperature;	/* 0.1*celsius */
	int interval;		/* ms */
};

#define ADC_SAMPLE_COUNT		5
#define ADC_SAMPLE_INTERVAL		10

#define PULL_UP_VOLTAGE_1800MV		1800	/* 1800mV */
#define PULL_UP_RESISTANCE_51KOHM	51000	/* 51kohm */

#define DETECT_INTERVAL_50MS		50	/* detect interval 50ms */
#define DETECT_INTERVAL_300MS		300	/* detect interval 300ms */
#define VALID_LOW_TEMPERATURE		200	/* 20C - 100C */
#define VALID_HIGH_TEMPERATURE		1000	/* 20C - 100C */
#define CRITICAL_TEMPERATURE		570	/* 57C */
#define CRITICAL_USBTEMP_ABOVE_BATTEMP	100	/* usb_temp - bat_temp >= 10C */
#define CRITICAL_RISE_TEMPERATURE	30	/* rise 3Celsius in 1500ms */
#define CRITICAL_RISE_INTERVAL		1500	/* rise 3Celsius in 1500ms */
#define TEMPERATURE_LIST_LENGTH		30	/* 1.5s / 50ms */

struct usb_temp temperature_list1[TEMPERATURE_LIST_LENGTH];	/* length = 1.5s / 50ms */
struct usb_temp temperature_list2[TEMPERATURE_LIST_LENGTH];	/* length = 1.5s / 50ms */

/*func:				check if dischg_enable
*temperature1 temperature2:	usb_temp (0.1*Celsius)
*critical_rise1 critical_rise2:	if rise 3Cesius in 1500ms
*bat_temp:			bat_temp (0.1*Celsius)
*dischg_enable:			dischg_enable		*/
#define check_dischg_enable(temperature1, temperature2, critical_rise1, critical_rise2, bat_temp, dischg_enable)	\
	do {														\
		dischg_enable = false;											\
		if (temperature1 >= VALID_LOW_TEMPERATURE && temperature1 <= VALID_HIGH_TEMPERATURE) {			\
			if (temperature1 >= CRITICAL_TEMPERATURE)							\
				dischg_enable = true;									\
			if (temperature1 - bat_temp >= CRITICAL_USBTEMP_ABOVE_BATTEMP && critical_rise1 == true)	\
				dischg_enable = true;									\
		}													\
		if (temperature2 >= VALID_LOW_TEMPERATURE && temperature2 <= VALID_HIGH_TEMPERATURE) {			\
			if (temperature2 >= CRITICAL_TEMPERATURE)							\
				dischg_enable = true;									\
			if (temperature2 - bat_temp >= CRITICAL_USBTEMP_ABOVE_BATTEMP && critical_rise2 == true)	\
				dischg_enable = true;									\
		}													\
	} while (0)

/*temp1 temp2:	usb_temp (0.1*Celsius)
*time:		thread interval (ms)
*i:		index of ntc_table	*/
#define add_temperature_list(temp1, temp2, time, i)							\
	do {												\
		for (i = TEMPERATURE_LIST_LENGTH - 1; i > 0; i --) {					\
			temperature_list1[i].temperature = temperature_list1[i - 1].temperature;	\
			temperature_list1[i].interval = temperature_list1[i - 1].interval;		\
			temperature_list2[i].temperature = temperature_list2[i - 1].temperature;	\
			temperature_list2[i].interval = temperature_list2[i - 1].interval;		\
		}											\
		temperature_list1[0].temperature = temp1;						\
		temperature_list1[0].interval = time;							\
		temperature_list2[0].temperature = temp2;						\
		temperature_list2[0].interval = time;							\
	} while (0)

/* check if rise 3Cesius within 1500ms */
#define check_temperature_list(list, total_time, critical_rise, i)					\
	do {												\
		critical_rise = false;									\
		total_time = 0;										\
		for (i = 0; i < TEMPERATURE_LIST_LENGTH; i ++) {					\
			total_time += list[i].interval;							\
			if (list[i].temperature == 0 && list[i].interval == 0)				\
				break;									\
			if (total_time - list[i].interval >= CRITICAL_RISE_INTERVAL)			\
				break;									\
			if (list[0].temperature - list[i].temperature >= CRITICAL_RISE_TEMPERATURE) {	\
				critical_rise = true;							\
				break;									\
			}										\
		}											\
	} while (0)

#define clear_temperature_list(i)					\
	do {								\
		for (i = 0; i < TEMPERATURE_LIST_LENGTH; i ++) {	\
			temperature_list1[i].temperature = 0;		\
			temperature_list1[i].interval = 0;		\
			temperature_list2[i].temperature = 0;		\
			temperature_list2[i].interval = 0;		\
		}							\
	} while (0)

/*R:	NTC resistance (ohm)
*T:	NTC temperature (0.1*Celsius)
*L:	ARRAY_SIZE(ntc_table)
*i:	index of ntc_table
*table:	ntc_table		*/
#define resistance_convert_temperature(R, T, i, table)			\
	do {								\
		for (i = 0; i < ARRAY_SIZE(table); i ++)		\
			if (table[i].resistance <= R)			\
				break;					\
		if (i == 0)						\
			T = table[0].temperature;			\
		else if (i == ARRAY_SIZE(table))			\
			T = table[ARRAY_SIZE(table) - 1].temperature;	\
		else						\
			T = table[i].temperature * (R - table[i - 1].resistance) / (table[i].resistance - table[i - 1].resistance) +	\
				table[i - 1].temperature * (table[i].resistance - R) / (table[i].resistance - table[i - 1].resistance);	\
	} while (0)

/*vadc:			usb_temp_adc voltage (mV)
*pull_up_voltage:	pull_up_voltage (mV)
*pull_up_resistance:	pull_up_resistance (ohm)	*/
#define voltage_convert_resistance(vadc, pull_up_voltage, pull_up_resistance)	\
	(pull_up_resistance / (pull_up_voltage - vadc) * vadc)

/* ntc1 100Kohm when 25c */
struct ntc_table ntc_table_100K[] = {
	/* {ohm, 0.1*celsius} */
	{4251000, -400},	{3962000, -390},	{3695000, -380},	{3447000, -370},	{3218000, -360},
	{3005000, -350},	{2807000, -340},	{2624000, -330},	{2454000, -320},	{2296000, -310},
	{2149000, -300},	{2012000, -290},	{1885000, -280},	{1767000, -270},	{1656000, -260},
	{1554000, -250},	{1458000, -240},	{1369000, -230},	{1286000, -220},	{1208000, -210},
	{1135000, -200},	{1068000, -190},	{1004000, -180},	{945000, -170},		{889600, -160},
	{837800, -150},		{789300, -140},		{743900, -130},		{701300, -120},		{661500, -110},
	{624100, -100},		{589000, -90},		{556200, -80},		{525300, -70},		{496300, -60},
	{469100, -50},		{443500, -40},		{419500, -30},		{396900, -20},		{375600, -10},
	{355600, 00},		{336800, 10},		{319100, 20},		{302400, 30},		{286700, 40},
	{271800, 50},		{257800, 60},		{244700, 70},		{232200, 80},		{220500, 90},
	{209400, 100},		{198900, 110},		{189000, 120},		{179700, 130},		{170900, 140},
	{162500, 150},		{154600, 160},		{147200, 170},		{140100, 180},		{133400, 190},
	{127000, 200},		{121000, 210},		{115400, 220},		{110000, 230},		{104800, 240},
	{100000, 250},		{95400, 260},		{91040, 270},		{86900, 280},		{82970, 290},
	{79230, 300},		{75690, 310},		{72320, 320},		{69120, 330},		{66070, 340},
	{63180, 350},		{60420, 360},		{57810, 370},		{55310, 380},		{52940, 390},
	{50680, 400},		{48530, 410},		{46490, 420},		{44530, 430},		{42670, 440},
	{40900, 450},		{39210, 460},		{37600, 470},		{36060, 480},		{34600, 490},
	{33190, 500},		{31860, 510},		{30580, 520},		{29360, 530},		{28200, 540},
	{27090, 550},		{26030, 560},		{25010, 570},		{24040, 580},		{23110, 590},
	{22220, 600},		{21370, 610},		{20560, 620},		{19780, 630},		{19040, 640},
	{18320, 650},		{17640, 660},		{16990, 670},		{16360, 680},		{15760, 690},
	{15180, 700},		{14630, 710},		{14100, 720},		{13600, 730},		{13110, 740},
	{12640, 750},		{12190, 760},		{11760, 770},		{11350, 780},		{10960, 790},
	{10580, 800},		{10210, 810},		{9859, 820},		{9522, 830},		{9198, 840},
	{8887, 850},		{8587, 860},		{8299, 870},		{8022, 880},		{7756, 890},
	{7500, 900},		{7254, 910},		{7016, 920},		{6788, 930},		{6568, 940},
	{6357, 950},		{6153, 960},		{5957, 970},		{5768, 980},		{5586, 990},
	{5410, 1000},		{5241, 1010},		{5078, 1020},		{4921, 1030},		{4769, 1040},
	{4623, 1050},		{4482, 1060},		{4346, 1070},		{4215, 1080},		{4088, 1090},
	{3966, 1100},		{3848, 1110},		{3734, 1120},		{3624, 1130},		{3518, 1140},
	{3415, 1150},		{3316, 1160},		{3220, 1170},		{3128, 1180},		{3038, 1190},
	{2952, 1200},		{2868, 1210},		{2787, 1220},		{2709, 1230},		{2634, 1240},
	{2561, 1250},
};

/* ntc2 10Kohm when 25c */
struct ntc_table ntc_table_10K[] = {
	/* {ohm, 0.1*celsius} */
	{0, 0},
};

int con_temp_855[] = {
	-40,
	-39,
	-38,
	-37,
	-36,
	-35,
	-34,
	-33,
	-32,
	-31,
	-30,
	-29,
	-28,
	-27,
	-26,
	-25,
	-24,
	-23,
	-22,
	-21,
	-20,
	-19,
	-18,
	-17,
	-16,
	-15,
	-14,
	-13,
	-12,
	-11,
	-10,
	-9,
	-8,
	-7,
	-6,
	-5,
	-4,
	-3,
	-2,
	-1,
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
	17,
	18,
	19,
	20,
	21,
	22,
	23,
	24,
	25,
	26,
	27,
	28,
	29,
	30,
	31,
	32,
	33,
	34,
	35,
	36,
	37,
	38,
	39,
	40,
	41,
	42,
	43,
	44,
	45,
	46,
	47,
	48,
	49,
	50,
	51,
	52,
	53,
	54,
	55,
	56,
	57,
	58,
	59,
	60,
	61,
	62,
	63,
	64,
	65,
	66,
	67,
	68,
};

int con_volt_855[] = {
	1779,
	1777,
	1775,
	1774,
	1772,
	1770,
	1768,
	1763,
	1761,
	1758,
	1756,
	1753,
	1750,
	1746,
	1743,
	1739,
	1735,
	1731,
	1727,
	1723,
	1718,
	1713,
	1708,
	1702,
	1697,
	1689,
	1678,
	1667,
	1655,
	1642,
	1630,
	1616,
	1603,
	1588,
	1573,
	1558,
	1542,
	1526,
	1509,
	1492,
	1474,
	1456,
	1438,
	1419,
	1399,
	1379,
	1359,
	1339,
	1318,
	1297,
	1275,
	1253,
	1231,
	1209,
	1187,
	1164,
	1142,
	1119,
	1096,
	1073,
	1050,
	1028,
	1005,
	982,
	959,
	937,
	915,
	893,
	871,
	849,
	827,
	806,
	785,
	764,
	744,
	724,
	704,
	685,
	666,
	647,
	629,
	611,
	593,
	576,
	559,
	543,
	527,
	511,
	496,
	481,
	467,
	453,
	439,
	426,
	413,
	400,
	388,
	376,
	364,
	353,
	342,
	332,
	321,
	311,
	302,
	302,
};

int con_temp_7250[] = {
	-40,
	-39,
	-38,
	-37,
	-36,
	-35,
	-34,
	-33,
	-32,
	-31,
	-30,
	-29,
	-28,
	-27,
	-26,
	-25,
	-24,
	-23,
	-22,
	-21,
	-20,
	-19,
	-18,
	-17,
	-16,
	-15,
	-14,
	-13,
	-12,
	-11,
	-10,
	-9,
	-8,
	-7,
	-6,
	-5,
	-4,
	-3,
	-2,
	-1,
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
	17,
	18,
	19,
	20,
	21,
	22,
	23,
	24,
	25,
	26,
	27,
	28,
	29,
	30,
	31,
	32,
	33,
	34,
	35,
	36,
	37,
	38,
	39,
	40,
	41,
	42,
	43,
	44,
	45,
	46,
	47,
	48,
	49,
	50,
	51,
	52,
	53,
	54,
	55,
	56,
	57,
	58,
	59,
	60,
	61,
	62,
	63,
	64,
	65,
	66,
	67,
	68,
	69,
	70,
	71,
	72,
	73,
	74,
	75,
	76,
	77,
	78,
	79,
	80,
	81,
	82,
	83,
	84,
	85,
	86,
	87,
	88,
	89,
	90,
	91,
	92,
	93,
	94,
	95,
	96,
	97,
	98,
	99,
	100,
};

int con_volt_7250[] = {
	1699,
	1696,
	1693,
	1689,
	1686,
	1682,
	1678,
	1674,
	1670,
	1665,
	1660,
	1655,
	1649,
	1644,
	1637,
	1631,
	1624,
	1617,
	1610,
	1602,
	1594,
	1586,
	1577,
	1568,
	1558,
	1548,
	1538,
	1527,
	1515,
	1504,
	1491,
	1479,
	1466,
	1452,
	1438,
	1424,
	1409,
	1394,
	1378,
	1362,
	1345,
	1328,
	1311,
	1293,
	1275,
	1256,
	1237,
	1218,
	1198,
	1178,
	1158,
	1138,
	1117,
	1096,
	1076,
	1054,
	1033,
	1012,
	990,
	969,
	947,
	926,
	904,
	883,
	861,
	840,
	819,
	798,
	777,
	756,
	736,
	715,
	695,
	676,
	656,
	637,
	618,
	599,
	581,
	563,
	545,
	528,
	511,
	495,
	478,
	462,
	447,
	432,
	417,
	403,
	389,
	375,
	362,
	349,
	336,
	324,
	312,
	300,
	289,
	278,
	267,
	257,
	247,
	237,
	228,
	219,
	210,
	201,
	193,
	185,
	177,
	170,
	162,
	155,
	149,
	142,
	136,
	129,
	123,
	118,
	112,
	107,
	102,
	96,
	92,
	87,
	82,
	78,
	74,
	70,
	66,
	62,
	58,
	54,
	51,
	48,
	44,
	41,
	38,
	35,
	32,
};

int con_volt_7250_svooc[] = {
	1759,
	1756,
	1753,
	1749,
	1746,
	1742,
	1738,
	1734,
	1730,
	1725,
	1720,
	1715,
	1709,
	1704,
	1697,
	1691,
	1684,
	1677,
	1670,
	1662,
	1654,
	1646,
	1637,
	1628,
	1618,
	1608,
	1598,
	1587,
	1575,
	1564,
	1551,
	1539,
	1526,
	1512,
	1498,
	1484,
	1469,
	1454,
	1438,
	1422,
	1405,
	1388,
	1371,
	1353,
	1335,
	1316,
	1297,
	1278,
	1258,
	1238,
	1218,
	1198,
	1177,
	1156,
	1136,
	1114,
	1093,
	1072,
	1050,
	1029,
	1007,
	986,
	964,
	943,
	921,
	900,
	879,
	858,
	837,
	816,
	796,
	775,
	755,
	736,
	716,
	697,
	678,
	659,
	641,
	623,
	605,
	588,
	571,
	555,
	538,
	522,
	507,
	492,
	477,
	463,
	449,
	435,
	422,
	409,
	396,
	384,
	372,
	360,
	349,
	338,
	327,
	317,
	307,
	297,
	288,
	279,
	270,
	261,
	253,
	245,
	237,
	230,
	222,
	215,
	209,
	202,
	196,
	189,
	183,
	178,
	172,
	167,
	162,
	156,
	152,
	147,
	142,
	138,
	134,
	130,
	126,
	122,
	118,
	114,
	111,
	108,
	104,
	101,
	98,
	95,
	92,
};

int con_temp_18097[] = {
	-40,
	-39,
	-38,
	-37,
	-36,
	-35,
	-34,
	-33,
	-32,
	-31,
	-30,
	-29,
	-28,
	-27,
	-26,
	-25,
	-24,
	-23,
	-22,
	-21,
	-20,
	-19,
	-18,
	-17,
	-16,
	-15,
	-14,
	-13,
	-12,
	-11,
	-10,
	-9,
	-8,
	-7,
	-6,
	-5,
	-4,
	-3,
	-2,
	-1,
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
	17,
	18,
	19,
	20,
	21,
	22,
	23,
	24,
	25,
	26,
	27,
	28,
	29,
	30,
	31,
	32,
	33,
	34,
	35,
	36,
	37,
	38,
	39,
	40,
	41,
	42,
	43,
	44,
	45,
	46,
	47,
	48,
	49,
	50,
	51,
	52,
	53,
	54,
	55,
	56,
	57,
	58,
	59,
	60,
	61,
	62,
	63,
	64,
	65,
	66,
	67,
	68,
	69,
	70,
	71,
	72,
	73,
	74,
	75,
	76,
	77,
	78,
	79,
	80,
	81,
	82,
	83,
	84,
	85,
	86,
	87,
	88,
	89,
	90,
	91,
	92,
	93,
	94,
	95,
	96,
	97,
	98,
	99,
	100,
	101,
	102,
	103,
	104,
	105,
	106,
	107,
	108,
	109,
	110,
	111,
	112,
	113,
	114,
	115,
	116,
	117,
	118,
	119,
	120,
	121,
	122,
	123,
	124,
	125,
};

int con_volt_18097[] = {
	1779,
	1777,
	1775,
	1774,
	1772,
	1770,
	1768,
	1766,
	1763,
	1761,
	1758,
	1756,
	1753,
	1750,
	1746,
	1743,
	1739,
	1735,
	1731,
	1727,
	1723,
	1718,
	1713,
	1708,
	1702,
	1697,
	1691,
	1685,
	1678,
	1671,
	1664,
	1657,
	1649,
	1641,
	1632,
	1623,
	1614,
	1605,
	1595,
	1585,
	1574,
	1563,
	1552,
	1540,
	1528,
	1516,
	1503,
	1490,
	1476,
	1462,
	1447,
	1433,
	1418,
	1402,
	1386,
	1370,
	1354,
	1337,
	1320,
	1302,
	1284,
	1266,
	1248,
	1230,
	1211,
	1192,
	1173,
	1154,
	1134,
	1115,
	1095,
	1075,
	1056,
	1036,
	1016,
	996,
	976,
	956,
	936,
	917,
	897,
	878,
	858,
	839,
	820,
	801,
	782,
	764,
	746,
	728,
	710,
	692,
	675,
	658,
	641,
	624,
	608,
	592,
	577,
	561,
	546,
	532,
	517,
	503,
	489,
	476,
	463,
	450,
	437,
	425,
	413,
	401,
	390,
	379,
	368,
	358,
	347,
	337,
	328,
	318,
	309,
	300,
	292,
	283,
	275,
	267,
	259,
	252,
	245,
	238,
	231,
	224,
	218,
	211,
	205,
	199,
	194,
	188,
	183,
	178,
	173,
	168,
	163,
	158,
	154,
	150,
	145,
	141,
	137,
	134,
	130,
	126,
	123,
	119,
	116,
	113,
	110,
	107,
	104,
	101,
	98,
	96,
	93,
	91,
	88,
	86,
};

int con_volt_18383[] = {
	2670,
	2668,
	2666,
	2664,
	2661,
	2658,
	2656,
	2652,
	2649,
	2646,
	2642,
	2638,
	2634,
	2630,
	2625,
	2621,
	2616,
	2610,
	2605,
	2599,
	2593,
	2586,
	2579,
	2572,
	2565,
	2557,
	2548,
	2540,
	2530,
	2521,
	2511,
	2500,
	2490,
	2478,
	2466,
	2454,
	2441,
	2428,
	2414,
	2400,
	2385,
	2369,
	2353,
	2337,
	2320,
	2302,
	2284,
	2265,
	2245,
	2226,
	2205,
	2184,
	2162,
	2140,
	2118,
	2094,
	2071,
	2047,
	2022,
	1997,
	1971,
	1945,
	1919,
	1892,
	1864,
	1837,
	1809,
	1781,
	1752,
	1724,
	1695,
	1666,
	1636,
	1607,
	1578,
	1548,
	1519,
	1489,
	1460,
	1430,
	1401,
	1372,
	1343,
	1314,
	1285,
	1256,
	1228,
	1200,
	1172,
	1145,
	1118,
	1091,
	1064,
	1038,
	1013,
	987,
	962,
	938,
	914,
	890,
	867,
	844,
	822,
	800,
	778,
	757,
	737,
	717,
	697,
	678,
	659,
	641,
	623,
	606,
	589,
	572,
	556,
	540,
	525,
	511,
	496,
	482,
	468,
	455,
	442,
	429,
	417,
	405,
	394,
	382,
	372,
	361,
	351,
	341,
	331,
	322,
	313,
	304,
	295,
	287,
	279,
	271,
	263,
	256,
	249,
	242,
	235,
	229,
	222,
	216,
	210,
	204,
	199,
	193,
	188,
	183,
	178,
	173,
	168,
	164,
	160,
	155,
	151,
	147,
	143,
	139,
};

int con_temp_18383[] = {
	-40,
	-39,
	-38,
	-37,
	-36,
	-35,
	-34,
	-33,
	-32,
	-31,
	-30,
	-29,
	-28,
	-27,
	-26,
	-25,
	-24,
	-23,
	-22,
	-21,
	-20,
	-19,
	-18,
	-17,
	-16,
	-15,
	-14,
	-13,
	-12,
	-11,
	-10,
	-9,
	-8,
	-7,
	-6,
	-5,
	-4,
	-3,
	-2,
	-1,
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
	17,
	18,
	19,
	20,
	21,
	22,
	23,
	24,
	25,
	26,
	27,
	28,
	29,
	30,
	31,
	32,
	33,
	34,
	35,
	36,
	37,
	38,
	39,
	40,
	41,
	42,
	43,
	44,
	45,
	46,
	47,
	48,
	49,
	50,
	51,
	52,
	53,
	54,
	55,
	56,
	57,
	58,
	59,
	60,
	61,
	62,
	63,
	64,
	65,
	66,
	67,
	68,
	69,
	70,
	71,
	72,
	73,
	74,
	75,
	76,
	77,
	78,
	79,
	80,
	81,
	82,
	83,
	84,
	85,
	86,
	87,
	88,
	89,
	90,
	91,
	92,
	93,
	94,
	95,
	96,
	97,
	98,
	99,
	100,
	101,
	102,
	103,
	104,
	105,
	106,
	107,
	108,
	109,
	110,
	111,
	112,
	113,
	114,
	115,
	116,
	117,
	118,
	119,
	120,
	121,
	122,
	123,
	124,
	125,
};

int con_volt_6125[] = {
	1832,
	1829,
	1826,
	1822,
	1818,
	1815,
	1811,
	1806,
	1802,
	1797,
	1792,
	1786,
	1781,
	1775,
	1768,
	1762,
	1755,
	1747,
	1740,
	1732,
	1723,
	1714,
	1705,
	1696,
	1686,
	1675,
	1664,
	1653,
	1641,
	1629,
	1616,
	1603,
	1589,
	1575,
	1561,
	1546,
	1530,
	1514,
	1498,
	1481,
	1463,
	1446,
	1428,
	1409,
	1390,
	1371,
	1351,
	1331,
	1311,
	1290,
	1269,
	1248,
	1226,
	1205,
	1183,
	1161,
	1139,
	1117,
	1094,
	1072,
	1049,
	1027,
	1005,
	982,
	959,
	938,
	915,
	894,
	872,
	850,
	829,
	808,
	787,
	766,
	746,
	726,
	706,
	687,
	668,
	649,
	631,
	613,
	595,
	578,
	561,
	544,
	528,
	512,
	497,
	482,
	467,
	453,
	439,
	426,
	412,
	400,
	387,
	375,
	363,
	352,
	341,
	330,
	320,
	310,
	300,
	290,
	281,
	272,
	264,
	255,
	247,
	239,
	232,
	224,
	217,
	210,
	204,
	197,
	191,
	185,
	179,
	174,
	168,
	163,
	158,
	153,
	148,
	144,
	139,
	135,
	131,
	127,
	123,
	119,
	116,
	112,
	109,
	105,
	102,
	99,
	96,
	93,
	91,
	88,
	85,
	83,
	80,
	78,
	76,
	74,
	72,
	69,
	67,
	66,
	64,
	62,
	60,
	58,
	57,
	55,
	54,
	52,
	51,
	49,
	48,
	47,
};

int con_temp_6125[] = {
	-40,
	-39,
	-38,
	-37,
	-36,
	-35,
	-34,
	-33,
	-32,
	-31,
	-30,
	-29,
	-28,
	-27,
	-26,
	-25,
	-24,
	-23,
	-22,
	-21,
	-20,
	-19,
	-18,
	-17,
	-16,
	-15,
	-14,
	-13,
	-12,
	-11,
	-10,
	-9,
	-8,
	-7,
	-6,
	-5,
	-4,
	-3,
	-2,
	-1,
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
	17,
	18,
	19,
	20,
	21,
	22,
	23,
	24,
	25,
	26,
	27,
	28,
	29,
	30,
	31,
	32,
	33,
	34,
	35,
	36,
	37,
	38,
	39,
	40,
	41,
	42,
	43,
	44,
	45,
	46,
	47,
	48,
	49,
	50,
	51,
	52,
	53,
	54,
	55,
	56,
	57,
	58,
	59,
	60,
	61,
	62,
	63,
	64,
	65,
	66,
	67,
	68,
	69,
	70,
	71,
	72,
	73,
	74,
	75,
	76,
	77,
	78,
	79,
	80,
	81,
	82,
	83,
	84,
	85,
	86,
	87,
	88,
	89,
	90,
	91,
	92,
	93,
	94,
	95,
	96,
	97,
	98,
	99,
	100,
	101,
	102,
	103,
	104,
	105,
	106,
	107,
	108,
	109,
	110,
	111,
	112,
	113,
	114,
	115,
	116,
	117,
	118,
	119,
	120,
	121,
	122,
	123,
	124,
	125,
};



int con_volt[] = {
	1721,
	1716,
	1710,
	1704,
	1697,
	1690,
	1683,
	1675,
	1667,
	1658,
	1649,
	1640,
	1630,
	1620,
	1609,
	1597,
	1586,
	1573,
	1560,
	1547,
	1533,
	1519,
	1504,
	1488,
	1472,
	1456,
	1438,
	1421,
	1403,
	1384,
	1365,
	1346,
	1326,
	1305,
	1285,
	1263,
	1242,
	1220,
	1198,
	1176,
	1153,
	1130,
	1107,
	1084,
	1061,
	1038,
	1014,
	991,
	967,
	944,
	921,
	898,
	875,
	852,
	829,
	807,
	785,
	763,
	741,
	720,
	699,
	678,
	658,
	638,
	619,
	600,
	581,
	563,
	545,
	527,
	510,
	494,
	477,
	462,
	446,
	432,
	417,
	403,
	389,
	376,
	363,
	351,
	339,
	327,
	316,
	305,
	295,
	284,
	274,
	265,
	256,
	247,
	238,
	230,
	222,
	214,
	207,
	200,
	193,
	186,
	180,
	173,
	167,
	162,
	156,
	151,
	145,
	140,
	136,
	131,
	127,
	122,
	118,
	114,
	110,
	106,
	103,
	99,
	96,
	93,
	90,
	87,
	84,
	81,
	79,
	76,
	73,
	71,
	69,
	67,
	64,
	62,
	60,
	58,
	57,
	55,
	53,
	51,
	50,
	48,
	47,
	45,
	44,
	42,
	41,
	40,
	39,
	38,
	36,
	35,
	34,
	33,
	32,
	31,
	30,
	29,
	29,
	28,
	27,
	26,
	25,
	25,
	24,
	23,
	23,
	22,
};

int con_temp[] = {
	-40,
	-39,
	-38,
	-37,
	-36,
	-35,
	-34,
	-33,
	-32,
	-31,
	-30,
	-29,
	-28,
	-27,
	-26,
	-25,
	-24,
	-23,
	-22,
	-21,
	-20,
	-19,
	-18,
	-17,
	-16,
	-15,
	-14,
	-13,
	-12,
	-11,
	-10,
	-9,
	-8,
	-7,
	-6,
	-5,
	-4,
	-3,
	-2,
	-1,
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
	17,
	18,
	19,
	20,
	21,
	22,
	23,
	24,
	25,
	26,
	27,
	28,
	29,
	30,
	31,
	32,
	33,
	34,
	35,
	36,
	37,
	38,
	39,
	40,
	41,
	42,
	43,
	44,
	45,
	46,
	47,
	48,
	49,
	50,
	51,
	52,
	53,
	54,
	55,
	56,
	57,
	58,
	59,
	60,
	61,
	62,
	63,
	64,
	65,
	66,
	67,
	68,
	69,
	70,
	71,
	72,
	73,
	74,
	75,
	76,
	77,
	78,
	79,
	80,
	81,
	82,
	83,
	84,
	85,
	86,
	87,
	88,
	89,
	90,
	91,
	92,
	93,
	94,
	95,
	96,
	97,
	98,
	99,
	100,
	101,
	102,
	103,
	104,
	105,
	106,
	107,
	108,
	109,
	110,
	111,
	112,
	113,
	114,
	115,
	116,
	117,
	118,
	119,
	120,
	121,
	122,
	123,
	124,
	125,
};

int con_volt_19165[] = {
	1668,
	1659,
	1651,
	1641,
	1632,
	1622,
	1611,
	1600,
	1589,
	1577,
	1565,
	1552,
	1539,
	1525,
	1511,
	1496,
	1481,
	1466,
	1449,
	1433,
	1416,
	1398,
	1381,
	1362,
	1344,
	1325,
	1305,
	1286,
	1266,
	1245,
	1225,
	1204,
	1183,
	1161,
	1140,
	1118,
	1096,
	1075,
	1053,
	1031,
	1009,
	987,
	965,
	943,
	922,
	900,
	879,
	857,
	836,
	815,
	795,
	774,
	754,
	734,
	715,
	695,
	677,
	658,
	640,
	622,
	604,
	587,
	570,
	554,
	537,
	522,
	506,
	491,
	477,
	462,
	449,
	435,
	422,
	409,
	397,
	385,
	373,
	361,
	350,
	339,
	329,
	319,
	309,
	300,
	290,
	281,
	273,
	264,
	256,
	248,
	241,
	233,
	226,
	219,
	212,
	206,
	200,
	193,
	188,
	182,
	176,
	171,
	166,
	161,
	156,
	151,
	147,
	142,
	138,
	134,
	130,
	126,
	123,
	119,
	116,
	112,
	109,
	106,
	103,
	100,
	97,
};

int con_temp_19165[] = {
	-20,
	-19,
	-18,
	-17,
	-16,
	-15,
	-14,
	-13,
	-12,
	-11,
	-10,
	-9,
	-8,
	-7,
	-6,
	-5,
	-4,
	-3,
	-2,
	-1,
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
	17,
	18,
	19,
	20,
	21,
	22,
	23,
	24,
	25,
	26,
	27,
	28,
	29,
	30,
	31,
	32,
	33,
	34,
	35,
	36,
	37,
	38,
	39,
	40,
	41,
	42,
	43,
	44,
	45,
	46,
	47,
	48,
	49,
	50,
	51,
	52,
	53,
	54,
	55,
	56,
	57,
	58,
	59,
	60,
	61,
	62,
	63,
	64,
	65,
	66,
	67,
	68,
	69,
	70,
	71,
	72,
	73,
	74,
	75,
	76,
	77,
	78,
	79,
	80,
	81,
	82,
	83,
	84,
	85,
	86,
	87,
	88,
	89,
	90,
	91,
	92,
	93,
	94,
	95,
	96,
	97,
	98,
	99,
	100,

};

int con_temp_7125[] = {
	-40,
	-39,
	-38,
	-37,
	-36,
	-35,
	-34,
	-33,
	-32,
	-31,
	-30,
	-29,
	-28,
	-27,
	-26,
	-25,
	-24,
	-23,
	-22,
	-21,
	-20,
	-19,
	-18,
	-17,
	-16,
	-15,
	-14,
	-13,
	-12,
	-11,
	-10,
	-9,
	-8,
	-7,
	-6,
	-5,
	-4,
	-3,
	-2,
	-1,
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
	17,
	18,
	19,
	20,
	21,
	22,
	23,
	24,
	25,
	26,
	27,
	28,
	29,
	30,
	31,
	32,
	33,
	34,
	35,
	36,
	37,
	38,
	39,
	40,
	41,
	42,
	43,
	44,
	45,
	46,
	47,
	48,
	49,
	50,
	51,
	52,
	53,
	54,
	55,
	56,
	57,
	58,
	59,
	60,
	61,
	62,
	63,
	64,
	65,
	66,
	67,
	68,
	69,
	70,
	71,
	72,
	73,
	74,
	75,
	76,
	77,
	78,
	79,
	80,
	81,
	82,
	83,
	84,
	85,
	86,
	87,
	88,
	89,
	90,
	91,
	92,
	93,
	94,
	95,
	96,
	97,
	98,
	99,
	100,

};
int con_volt_7125[] = {
	1758,
	1755,
	1752,
	1749,
	1745,
	1742,
	1738,
	1733,
	1729,
	1724,
	1719,
	1714,
	1709,
	1703,
	1697,
	1691,
	1684,
	1677,
	1670,
	1662,
	1654,
	1645,
	1636,
	1627,
	1618,
	1608,
	1597,
	1586,
	1575,
	1563,
	1551,
	1538,
	1525,
	1512,
	1498,
	1483,
	1468,
	1453,
	1437,
	1421,
	1416,
	1398,
	1381,
	1362,
	1344,
	1325,
	1305,
	1286,
	1266,
	1245,
	1225,
	1204,
	1183,
	1161,
	1140,
	1118,
	1096,
	1075,
	1053,
	1031,
	1009,
	987,
	965,
	943,
	922,
	900,
	879,
	857,
	836,
	815,
	795,
	774,
	754,
	734,
	715,
	695,
	677,
	658,
	640,
	622,
	604,
	587,
	570,
	554,
	537,
	522,
	506,
	491,
	477,
	462,
	449,
	435,
	422,
	409,
	397,
	385,
	373,
	361,
	350,
	339,
	329,
	319,
	309,
	300,
	290,
	281,
	273,
	264,
	256,
	248,
	241,
	233,
	226,
	219,
	212,
	206,
	200,
	193,
	188,
	182,
	176,
	171,
	166,
	161,
	156,
	151,
	147,
	142,
	138,
	134,
	130,
	126,
	123,
	119,
	116,
	112,
	109,
	106,
	103,
	100,
	97,

};
#endif /* __OP_CHARGE_H__ */
