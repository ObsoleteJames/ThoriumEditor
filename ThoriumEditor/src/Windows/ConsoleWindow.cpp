
#include "ConsoleWindow.h"
#include "EditorEngine.h"

#include <QLabel>
#include <QBoxLayout>
#include <QTextEdit>
#include <QLineEdit>

const char* logTypeText[] = {
	"",
	"[INF]",
	"[WRN]",
	"[ERR]"
};

static FString TimeToHmsString(time_t* time)
{
	struct tm time_info;
	char timeString[9];  // space for "HH:MM:SS\0"

	localtime_s(&time_info, time);

	strftime(timeString, sizeof(timeString), "%H:%M:%S", &time_info);
	timeString[8] = '\0';
	return FString(timeString);
}

CConsoleWidget::CConsoleWidget(QWidget* parent /*= nullptr*/) : ads::CDockWidget("Console", parent)
{
	QWidget* rootWidget = new QWidget(this);
	QVBoxLayout* layout = new QVBoxLayout();
	rootWidget->setLayout(layout);

	setIcon(QIcon(":/icons/wnd_console.svg"));

	setObjectName("console_widget");
	//setWindowTitle("Console");

	setWidget(rootWidget);

	consoleLog = new QTextEdit(this);
	consoleLog->setReadOnly(true);
	layout->addWidget(consoleLog);

	consoleLog->setFontFamily("Consolas");

	input = new QLineEdit(this);
	input->setProperty("type", QVariant(1));
	input->setFont(consoleLog->font());

	layout->addWidget(input);

	const auto& logPtr = CConsole::GetMsgCache();
	for (auto log : logPtr)
	{
		OnLog(log);
	}

	onLogBinding = CConsole::GetLogEvent().Bind(this, &CConsoleWidget::OnLog);

	connect(input, &QLineEdit::returnPressed, this, [=]() { 
		class ConsoleExecEvent : public IEditorEvent
		{
		public:
			ConsoleExecEvent(const std::string& i) : input(i) {}

			void Exec() override
			{
				CConsole::Exec(input);
			}

			std::string input;
		};
		ConsoleExecEvent* event = new ConsoleExecEvent(input->text().toStdString());
		gEditorEngine()->PushEvent(event);

		input->clear();
	});
}

CConsoleWidget::~CConsoleWidget()
{
	CConsole::GetLogEvent().Remove(onLogBinding);
	delete consoleLog;
}

void CConsoleWidget::OnLog(const FConsoleMsg& msg)
{
	consoleLog->moveCursor(QTextCursor::End);
	consoleLog->setTextColor(QColor(111, 179, 75));
	//consoleLog->setTextBackgroundColor(QColor(111, 179, 75, 20));
	//consoleLog->insertPlainText(logTypeText[msg.type]);
	consoleLog->setTextBackgroundColor(QColor(0, 0, 0, 0));

	FString msgTime = TimeToHmsString((time_t*)&msg.time);
	consoleLog->insertPlainText(("[" + msgTime + "] " + msg.module).c_str());
	
	consoleLog->setTextBackgroundColor(QColor(0, 0, 0, 0));
	consoleLog->setTextColor(QColor(200, 200, 200));
	//consoleLog->setTextColor(QColor("text"));

	consoleLog->insertPlainText(" ");

	if (msg.type == CONSOLE_WARNING)
		consoleLog->setTextColor(QColor(230, 197, 67));

	if (msg.type == CONSOLE_ERROR)
		consoleLog->setTextBackgroundColor(QColor(207, 32, 23, 100));

	consoleLog->insertPlainText((msg.msg + "\n").c_str());
	consoleLog->moveCursor(QTextCursor::End);
	consoleLog->setTextBackgroundColor(QColor(0, 0, 0, 0));
}
