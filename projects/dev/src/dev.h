
#include "vast.h"

using namespace vast;

class Dev : public IApp
{
public:
	Dev() {}
	~Dev() {}

	void OnWindowClose();
	void OnWindowResize(const EventData& event);

	void Run() override;
};