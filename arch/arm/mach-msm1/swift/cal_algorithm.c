/* ---------------------------------------------------------------------- //
//
// 29.09.2008 JunYeong Han
// Calibration Algorithm Source File
//
// ---------------------------------------------------------------------- */

#define NUM_OF_POINT		5
#define SHIFT_NUM			28

/* ---------------------------------------------------------------------- //
// Variable for first equation.
// ---------------------------------------------------------------------- */

static int g_iSecondDelta;
static int g_iSecondDeltaX1;
static int g_iSecondDeltaX2;
static int g_iSecondDeltaX3;
static int g_iSecondDeltaY1;
static int g_iSecondDeltaY2;
static int g_iSecondDeltaY3;

/* ---------------------------------------------------------------------- //
// Variable for second equation.
// ---------------------------------------------------------------------- */

static long long g_dSecondDelta;
static long long g_dSecondDeltaX1;
static long long g_dSecondDeltaX2;
static long long g_dSecondDeltaX3;
static long long g_dSecondDeltaY1;
static long long g_dSecondDeltaY2;
static long long g_dSecondDeltaY3;

/* ---------------------------------------------------------------------- //
// Variable for third equation.
// ---------------------------------------------------------------------- */

static long long g_dThirdA;
static long long g_dThirdB;
static long long g_dThirdC;
static long long g_dThirdD;
static long long g_dThirdE;

static long long g_dThirdX1;
static long long g_dThirdX2;
static long long g_dThirdX3;
static long long g_dThirdY1;
static long long g_dThirdY2;
static long long g_dThirdY3;

/* ---------------------------------------------------------------------- //
// Variable for input values.
// ---------------------------------------------------------------------- */

static int g_aiDisplayX[NUM_OF_POINT];
static int g_aiDisplayY[NUM_OF_POINT];
static int g_aiInputX[NUM_OF_POINT];
static int g_aiInputY[NUM_OF_POINT];

static void CalculateThirdEquations(void);
static void CalculateSecondEquations(void);

void Calibrate(int iX, int iY, int* pRetX, int* pRetY)
{
	*pRetX = iX * g_iSecondDeltaX1 / g_iSecondDelta + 
		iY * g_iSecondDeltaX2 / g_iSecondDelta +
		g_iSecondDeltaX3 / g_iSecondDelta;
	*pRetY = iY * g_iSecondDeltaY1 / g_iSecondDelta +
		iY * g_iSecondDeltaY2 / g_iSecondDelta + 
		g_iSecondDeltaY3 / g_iSecondDelta;

	if(*pRetX < 1)
	{
		*pRetX = 1;
	}
	else if(*pRetX > 800)
	{
		*pRetX = 800;
	}
	
	if(*pRetY < 1)
	{
		*pRetY = 1;
	}
	else if(*pRetY > 800)
	{
		*pRetY = 800;
	}
}

void SetInputValue(int* aiDisplayX, int* aiDisplayY, int* aiInputX, int* aiInputY)
{
	g_aiDisplayX[0] = aiDisplayX[0];
	g_aiDisplayX[1] = aiDisplayX[1];
	g_aiDisplayX[2] = aiDisplayX[2];
	g_aiDisplayX[3] = aiDisplayX[3];
	g_aiDisplayX[4] = aiDisplayX[4];

	g_aiDisplayY[0] = aiDisplayY[0];
	g_aiDisplayY[1] = aiDisplayY[1];
	g_aiDisplayY[2] = aiDisplayY[2];
	g_aiDisplayY[3] = aiDisplayY[3];
	g_aiDisplayY[4] = aiDisplayY[4];

	g_aiInputX[0] = aiInputX[0];
	g_aiInputX[1] = aiInputX[1];
	g_aiInputX[2] = aiInputX[2];
	g_aiInputX[3] = aiInputX[3];
	g_aiInputX[4] = aiInputX[4];

	g_aiInputY[0] = aiInputY[0];
	g_aiInputY[1] = aiInputY[1];
	g_aiInputY[2] = aiInputY[2];
	g_aiInputY[3] = aiInputY[3];
	g_aiInputY[4] = aiInputY[4];

	CalculateThirdEquations();
	CalculateSecondEquations();
}

static void CalculateSecondEquations(void)
{
	g_dSecondDelta = NUM_OF_POINT * (g_dThirdA * g_dThirdB - g_dThirdC * g_dThirdC) +
		2 * g_dThirdC * g_dThirdD * g_dThirdE - g_dThirdA * g_dThirdE * g_dThirdE -
		g_dThirdB * g_dThirdD * g_dThirdD;
	
	g_dSecondDeltaX1 = NUM_OF_POINT * (g_dThirdX1 * g_dThirdB - g_dThirdX2 * g_dThirdC) +
		g_dThirdE * (g_dThirdX2 * g_dThirdD - g_dThirdX1 * g_dThirdE) + 
		g_dThirdX3 * (g_dThirdC * g_dThirdE - g_dThirdB * g_dThirdD);
	
	g_dSecondDeltaX2 = NUM_OF_POINT * (g_dThirdX2 * g_dThirdA - g_dThirdX1 * g_dThirdC) +
		g_dThirdD * (g_dThirdX1 * g_dThirdE - g_dThirdX2 * g_dThirdD) +
		g_dThirdX3 * (g_dThirdC * g_dThirdD - g_dThirdA * g_dThirdE);
	
	g_dSecondDeltaX3 = g_dThirdX3 * (g_dThirdA * g_dThirdB - g_dThirdC * g_dThirdC) +
		g_dThirdX1 * (g_dThirdC * g_dThirdE - g_dThirdB * g_dThirdD) + 
		g_dThirdX2 * (g_dThirdC * g_dThirdD - g_dThirdA * g_dThirdE);
	
	g_dSecondDeltaY1 = NUM_OF_POINT * (g_dThirdY1 * g_dThirdB - g_dThirdY2 * g_dThirdC) +
		g_dThirdE * (g_dThirdY2 * g_dThirdD - g_dThirdY1 * g_dThirdE) + 
		g_dThirdY3 * (g_dThirdC * g_dThirdE - g_dThirdB * g_dThirdD);
	
	g_dSecondDeltaY2 = NUM_OF_POINT * (g_dThirdY2 * g_dThirdA - g_dThirdY1 * g_dThirdC) +
		g_dThirdD * (g_dThirdY1 * g_dThirdE - g_dThirdY2 * g_dThirdD) +
		g_dThirdY3 * (g_dThirdC * g_dThirdD - g_dThirdA * g_dThirdE);
	
	g_dSecondDeltaY3 = g_dThirdY3 * (g_dThirdA * g_dThirdB - g_dThirdC * g_dThirdC) +
		g_dThirdY1 * (g_dThirdC * g_dThirdE - g_dThirdB * g_dThirdD) + 
		g_dThirdY2 * (g_dThirdC * g_dThirdD - g_dThirdA * g_dThirdE);

	if(g_dSecondDelta < 0)
	{
		g_dSecondDelta *= -1;
		g_iSecondDelta = g_dSecondDelta >> SHIFT_NUM;
		g_iSecondDelta *= -1;
	}
	else
	{
		g_iSecondDelta = g_dSecondDelta >> SHIFT_NUM;
	}

	if(g_dSecondDeltaX1 < 0)
	{
		g_dSecondDeltaX1 *= -1;
		g_iSecondDeltaX1 = g_dSecondDeltaX1 >> SHIFT_NUM;
		g_iSecondDeltaX1 *= -1;
	}
	else
	{
		g_iSecondDeltaX1 = g_dSecondDeltaX1 >> SHIFT_NUM;
	}
	
	if(g_dSecondDeltaX2 < 0)
	{
		g_dSecondDeltaX2 *= -1;
		g_iSecondDeltaX2 = g_dSecondDeltaX2 >> SHIFT_NUM;
		g_iSecondDeltaX2 *= -1;
	}
	else
	{
		g_iSecondDeltaX2 = g_dSecondDeltaX2 >> SHIFT_NUM;
	}

	if(g_dSecondDeltaX3 < 0)
	{
		g_dSecondDeltaX3 *= -1;
		g_iSecondDeltaX3 = g_dSecondDeltaX3 >> SHIFT_NUM;
		g_iSecondDeltaX3 *= -1;
	}
	else
	{
		g_iSecondDeltaX3 = g_dSecondDeltaX3 >> SHIFT_NUM;
	}

	if(g_dSecondDeltaY1 < 0)
	{
		g_dSecondDeltaY1 *= -1;
		g_iSecondDeltaY1 = g_dSecondDeltaY1 >> SHIFT_NUM;
		g_iSecondDeltaY1 *= -1;
	}
	else
	{
		g_iSecondDeltaY1 = g_dSecondDeltaY1 >> SHIFT_NUM;
	}
	
	if(g_dSecondDeltaY2 < 0)
	{
		g_dSecondDeltaY2 *= -1;
		g_iSecondDeltaY2 = g_dSecondDeltaY2 >> SHIFT_NUM;
		g_iSecondDeltaY2 *= -1;
	}
	else
	{
		g_iSecondDeltaY2 = g_dSecondDeltaY2 >> SHIFT_NUM;
	}

	if(g_dSecondDeltaY3 < 0)
	{
		g_dSecondDeltaY3 *= -1;
		g_iSecondDeltaY3 = g_dSecondDeltaY3 >> SHIFT_NUM;
		g_iSecondDeltaY3 *= -1;
	}
	else
	{
		g_iSecondDeltaY3 = g_dSecondDeltaY3 >> SHIFT_NUM;
	}
}

static void CalculateThirdEquations(void)
{
	int i = 0;

	g_dThirdA = 0;
	g_dThirdB = 0;
	g_dThirdC = 0;
	g_dThirdD = 0;
	g_dThirdE = 0;

	g_dThirdX1 = 0;
	g_dThirdX2 = 0;
	g_dThirdX3 = 0;
	
	g_dThirdY1 = 0;
	g_dThirdY2 = 0;
	g_dThirdY3 = 0;

	for(i = 0; i < NUM_OF_POINT; ++i)
	{
		g_dThirdA += g_aiInputX[i] * g_aiInputX[i];
		g_dThirdB += g_aiInputY[i] * g_aiInputY[i];
		g_dThirdC += g_aiInputX[i] * g_aiInputY[i];
		g_dThirdD += g_aiInputX[i];
		g_dThirdE += g_aiInputY[i];

		g_dThirdX1 += g_aiInputX[i] * g_aiDisplayX[i];
		g_dThirdX2 += g_aiInputY[i] * g_aiDisplayX[i];
		g_dThirdX3 += g_aiDisplayX[i];

		g_dThirdY2 += g_aiInputX[i] * g_aiDisplayY[i];
		g_dThirdY1 += g_aiInputY[i] * g_aiDisplayY[i];
		g_dThirdY3 += g_aiDisplayY[i];
	}
}

