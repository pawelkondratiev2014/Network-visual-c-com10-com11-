#include <iostream>
#include <windows.h>
#include <direct.h>
#include <conio.h>


#define SYN		0x16		//синхронизирующий символ
#define SOH		0x01		//начало заголовка пакета
#define STX		0x02		//начало текста
#define ETX		0x03		//конец текста
#define ETB		0x17		//конец передачи блока (заголовка)
#define ENQ		0x05		//запрос
#define ACK0		0x06		//четное подтверждение
#define ACK1		0x08		//нечетное подтверждение
#define NAK		0x15		//отрицательная квитанция (переспрос)
#define EOT		0x04		//конец сеанса связи
#define DLE		0x10		//спец. управляющий символ

HANDLE hCOM;
bool dle;
unsigned char f1;
char text[1024];
int text_pos;

void reset()
{
	dle = false;
	f1 = 0;
	text_pos = 0;
	memset(text, 0, sizeof(text));
	std::cout << "\nSPACE - начать передачу, ESC - выход\nОжидаем...\n";
};

void send_char(const char c, bool double_dle = true)
{
	DWORD nb = 0;
	if (c == DLE && double_dle)
		WriteFile(hCOM, &c, sizeof(c), &nb, 0); //пишет данные в файл
	WriteFile(hCOM, &c, sizeof(c), &nb, 0);
};

void send_control(const char c)
{
	send_char(DLE, false);
	send_char(c);
};

void close()
{
	CloseHandle(hCOM);
	ExitProcess(0);
};

void open_port(char* portname)
{
	hCOM = CreateFile(portname,
		GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, 0, NULL);

	if (hCOM == INVALID_HANDLE_VALUE)
	{
		std::cout << "Ошибка открытия порта" << std::endl;
		close();
	};

	const int TIMEOUT = 1000;
	COMMTIMEOUTS CommTimeOuts;
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = TIMEOUT;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = TIMEOUT;
	if (!SetCommTimeouts(hCOM, &CommTimeOuts)) {
		std::cout << "Ошибка настройки порта" << std::endl;
		close();
	};

};


bool recv()
{
	unsigned char c = 0;
	DWORD nb = 0;

	bool res = ReadFile(hCOM, &c, sizeof(c), &nb, NULL);
	res = res && nb > 0;

	//Читаем из порта 1 байт
	if (res)
	{
		if (!dle)
		{
			f1 = 0;
			if (c == DLE)
				dle = true;
			else
				text[text_pos++] = c;
		}
		//Ожидается управляющий символ
		else
		{
			//Обработка управляющих символов
			if (c == DLE)
			{
				//Обработать DLE как обычный символ
				text[text_pos++] = c;
			}
			else
			{
				f1 = c;
				if (c == SOH || c == STX)
				{
					text_pos = 0;
					memset(text, 0, sizeof(text));
				}
			}
			dle = false;
		}
		printf("%.2x\t%c\n", c, c);
	}

	return res;
};

void send_text(const char* str)
{
	for (int i = 0; i < strlen(str); i++)
		send_char(str[i]);
};

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, ".1251");
	if (argc != 2)
	{
		std::cout << "Не задано имя порта" << std::endl;
		return 0;
	};
	std::cout << argv[1] << std::endl;
	char portname[MAX_PATH] = "\\\\.\\";
	strncat_s(portname,argv[1], MAX_PATH);
	char headerbuff[] = "Header text";
	char databuff[MAX_PATH];
	_getcwd(databuff, sizeof(databuff));
	std::cout << "Открываем порт " << portname << std::endl;
	open_port(portname);
	reset();
	
	do {	

		if (_kbhit())
		{
			char c = _getch();
			if (c == 32)
				send_control(ENQ);
			if (c == 27)
				close();
		}
		else
				
			
		if (recv())
		{
			switch (f1)
			{
			case ENQ:
				send_control(ACK0);
				break;
			case ACK0:
				send_control(SOH);
				send_text(headerbuff);
				send_control(ETB);
				break;
			case ETB:
				std::cout << text << std::endl;
				send_control(ACK1);
				break;
			case ACK1:
				send_control(STX);
				send_text(databuff);
				send_control(ETX);
				break;
			case ETX:
				std::cout << text << std::endl;
				send_control(EOT);
				reset();
				break;
			case EOT:
				reset();
				break;
			}
		}






	} while (true);

	return 0;
}