#pragma once

#include "stdstring.h"
namespace HTML
{
	class CHTMLUtil
	{
	public:
		CHTMLUtil(void);
		virtual ~CHTMLUtil(void);
		int FindTag(const CStdString& strHTML,const CStdString& strTag,CStdString& strtagFound, int iPos=0) const;
		void getValueOfTag(const CStdString& strTagAndValue, CStdString& strValue);
		void getAttributeOfTag(const CStdString& strTagAndValue,const CStdString& strTag, CStdString& strValue);
		void RemoveTags(CStdString& strHTML);
		void ConvertHTMLToAnsi(const CStdString& strHTML, CStdString& strStripped);
	};

};