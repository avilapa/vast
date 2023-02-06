
#include "vast.h"

class Dev : public vast::IApp
{
public:
	Dev() {}
	~Dev() {}

	void OnWindowClose();
	void OnWindowResize(const vast::EventData& event);

	void Run() override;
};