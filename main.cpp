#include <egt/ui>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>

#include "rfProc.h"

using namespace std;
using namespace egt;

#define TEMP_MAX	35
#define TEMP_MIN	15

int gTemp = 20;

#define LIGHT_ON	1
#define LIGHT_OFF	0
int gLight = LIGHT_OFF;

#define FAN_SPEED_0		0
#define FAN_SPEED_1		1
#define FAN_SPEED_2		2
#define FAN_SPEED_3		3
int gFan	= FAN_SPEED_0;

int gCurTemp = -99;

pthread_t tRF;

char gTempBuffer[10];

void ShowTemp(unsigned char integer, unsigned char dec)
{
	if( dec >= 10 && dec < 100)
		dec /= 10;
	else if( dec >= 100 )
		dec /=100;

	sprintf(gTempBuffer, "%d.%1d", integer, dec);
}

int main(int argc, char **argv)
{
	Application app;
	TopWindow window;

	char buf[10];

	int ret;

	ret = pthread_create(&tRF, NULL, rfProc, NULL);
	if( ret != 0 )
	{
		printf("Create UART thread fail\r\n");
		return -1;
	}

	rfSetCallback(ShowTemp);

	// image lable process
	ImageLabel LightStatus(window, Image("file:./resources/light_off.png"));
	LightStatus.move(Point(215, 385));

	ImageLabel EgtLogo(window, Image("file:./resources/egt_logo.png"));
	EgtLogo.move(Point(16, 13));

	ImageLabel FanStatus(window, Image("file:./resources/speed0.png"));
	FanStatus.move(Point(340, 415));

	sprintf(buf, "%d", gTemp);
	Label lbTemp(window, buf);
	lbTemp.move(Point(690, 130));
	Font tempFont("Serif", 80, Font::Weight::bold, Font::Slant::normal);
	lbTemp.font(tempFont);
	lbTemp.color(Palette::ColorId::label_text, Palette::white);
	lbTemp.color(Palette::ColorId::label_bg, Palette::black);

	if( gCurTemp == -99)
	{
		sprintf(buf, "--.-");
	}
	else
	{
		sprintf(buf, "%d", gCurTemp);
	}
	Label lbcurTemp(window, buf);
	lbcurTemp.text(buf);
	lbcurTemp.move(Point(120, 70));
	Font tempCurFont("Serif", 220, Font::Weight::bold, Font::Slant::normal);
	lbcurTemp.font(tempCurFont);
	lbcurTemp.color(Palette::ColorId::label_text, Palette::blue);
	lbcurTemp.color(Palette::ColorId::label_bg, Palette::black);

	ImageButton btnLight(window, Image("file:./resources/light.png"));
	btnLight.move(Point(15, 405));
	btnLight.fill_flags().clear();
	btnLight.image_align(egt::AlignFlag::expand);

	btnLight.on_event([ & ](Event & event)
	{
		if( event.id() == EventId::pointer_click)
		{
			cout << "light control" << endl;

			if( gLight==LIGHT_ON )
			{
				gLight = LIGHT_OFF;
				LightStatus.image(Image("file:./resources/light_off.png"));
				rfSend(COMMAND_LIGHT_OFF, 0);
			}
			else
			{
				gLight = LIGHT_ON;
				LightStatus.image(Image("file:./resources/light_on.png"));
				rfSend(COMMAND_LIGHT_ON, 0);
			}
		}
		else if( event.id() == EventId::raw_pointer_down)
		{
			btnLight.image(Image("file:./resources/lightP.png"));
			cout<<"down!!"<<endl;
		}
		else if( event.id() == EventId::raw_pointer_up)
		{
			btnLight.image(Image("file:./resources/light.png"));
			cout<<"up!!"<<endl;
		}
	});

	ImageButton btnFan(window, Image("file:./resources/fan.png"));
	btnFan.move(Point(480, 380));
	btnFan.fill_flags().clear();
	btnFan.image_align(egt::AlignFlag::expand);

	btnFan.on_event([ & ](Event & event)
	{
		if( event.id() == EventId::pointer_click)
		{
			int err = 0;
			cout << "fan control" << endl;
			switch( gFan )
			{
				case FAN_SPEED_0:
					gFan = FAN_SPEED_1;
					FanStatus.image(Image("file:./resources/speed1.png"));
					break;

				case FAN_SPEED_1:
					gFan = FAN_SPEED_2;
					FanStatus.image(Image("file:./resources/speed2.png"));
					break;
				case FAN_SPEED_2:
					gFan = FAN_SPEED_3;
					FanStatus.image(Image("file:./resources/speed3.png"));
					break;
				case FAN_SPEED_3:
					gFan = FAN_SPEED_0;
					FanStatus.image(Image("file:./resources/speed0.png"));
					break;
				default:
					err = 1;
					break;
			}


			if ( err == 0 )
			{
				rfSend(COMMAND_FANS_CTRL, gFan);
			}
		}
		else if( event.id() == EventId::raw_pointer_down)
		{
			btnFan.image(Image("file:./resources/fanP.png"));
			cout<<"down!!"<<endl;
		}
		else if( event.id() == EventId::raw_pointer_up)
		{
			btnFan.image(Image("file:./resources/fan.png"));
			cout<<"up!!"<<endl;
		}

	});

	ImageButton btnUp(window, Image("file:./resources/btnUp.png"));
	btnUp.move(Point(690, 40));
	btnUp.fill_flags().clear();
	btnUp.image_align(egt::AlignFlag::expand);

	btnUp.on_event([ & ](Event & event)
	{
		if( event.id() == EventId::pointer_click)
		{
			cout << "temp up" << endl;

			if( gTemp < TEMP_MAX )
			{
				gTemp++;
				sprintf(buf, "%d", gTemp);
				lbTemp.text(buf);
				rfSend(COMMAND_TEMP_SET, gTemp);
			}
		}
		else if( event.id() == EventId::raw_pointer_down)
		{
			btnUp.image(Image("file:./resources/btnUpP.png"));
			cout<<"down!!"<<endl;
		}
		else if( event.id() == EventId::raw_pointer_up)
		{
			btnUp.image(Image("file:./resources/btnUp.png"));
			cout<<"up!!"<<endl;
		}
	});

	ImageButton btnDown(window, Image("file:./resources/btnDown.png"));
	btnDown.move(Point(690, 240));
	btnDown.fill_flags().clear();
	btnDown.image_align(egt::AlignFlag::expand);

	btnDown.on_event([ & ](Event & event)
	{
		if( event.id() == EventId::pointer_click)
		{
			cout << "temp down" << endl;
			if( gTemp > TEMP_MIN )
			{
				gTemp--;
				sprintf(buf, "%d", gTemp);
				lbTemp.text(buf);
				rfSend(COMMAND_TEMP_SET, gTemp);
			}
		}
		else if( event.id() == EventId::raw_pointer_down)
		{
			btnDown.image(Image("file:./resources/btnDownP.png"));
			cout<<"down!!"<<endl;
		}
		else if( event.id() == EventId::raw_pointer_up)
		{
			btnDown.image(Image("file:./resources/btnDown.png"));
			cout<<"up!!"<<endl;
		}
	});

	egt::PeriodicTimer cputimer(std::chrono::seconds(1));
	cputimer.on_timeout([&lbcurTemp]()
	{
		lbcurTemp.text(gTempBuffer);
	});
	cputimer.start();

	// window process
	window.background(Image("file:./resources/background.png"));
	window.show();

	return app.run();
}

// end of file
