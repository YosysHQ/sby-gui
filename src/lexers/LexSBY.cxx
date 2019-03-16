#include <assert.h>

#include "LexSBY.h"

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"

using namespace Scintilla;

static void ColouriseSBYDoc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordLists[], Accessor &styler)
{
	StyleContext sc(startPos, length, initStyle, styler);
	for (; sc.More(); sc.Forward())
	{
		if (sc.ch == '\r' || sc.ch == '\n') {
			sc.SetState(SCE_SBY_DEFAULT);
			continue;
		}
		switch (sc.state)
		{
			case SCE_SBY_DEFAULT:
				if (sc.ch == '-' && sc.chNext == '-')
					sc.SetState(SCE_SBY_COMMENT);
				else if (sc.ch == '[')
					sc.SetState(SCE_SBY_SECTION);
				break;
			case SCE_SBY_COMMENT:
				break;
			case SCE_SBY_SECTION:
				if (sc.ch == ']')
					sc.ForwardSetState(SCE_SBY_DEFAULT);
				break;
		}
	}
	sc.Complete();
}

static const char *const emptyWordListDesc[] = {
	0
};

LexerModule lmSBY(SCLEX_SBY, ColouriseSBYDoc, "sby", nullptr, emptyWordListDesc);
