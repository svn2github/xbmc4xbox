#pragma once
#include "iosdoption.h"
#include "GUIImage.h"
namespace OSD
{
	class COSDOptionBoolean :
		public IOSDOption
	{
	public:
		COSDOptionBoolean(int iHeading);
		COSDOptionBoolean(int iHeading,bool bValue);
		COSDOptionBoolean(const COSDOptionBoolean& option);
		const OSD::COSDOptionBoolean& operator = (const COSDOptionBoolean& option);


		virtual ~COSDOptionBoolean(void);
		virtual IOSDOption* Clone() const;
		virtual void Draw(int x, int y, bool bFocus=false, bool bSelected=false);
		virtual bool OnAction(const CAction& action);
	private:
		bool	      m_bValue;
    CGUIImage   m_image;
	};
};
