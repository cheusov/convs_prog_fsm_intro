############################################################

MKC_REQUIRE_PROGS =	${PS2PDF} ${DOT} ${DVIPS} ${LATEX}

FILES             =	fsm_intro.pdf
FILESDIR          =	${DOCDIR}

############################################################

PS2PDF              =	ps2pdf
DOT                 =	dot
DVIPS               =   dvips
LATEX               =	latex

MKC_REQUIRE_PROGS   =	${PS2PDF} ${DOT} ${DVIPS} ${LATEX}

do-all : fsm_intro.pdf

.PHONY : pdf dvi rtf
pdf : fsm_intro.pdf
ps  : fsm_intro.ps
dvi : fsm_intro.dvi

.SUFFIXES: .ps .eps .pdf .dvi .tex .dot

fsa_list = fsa_example fsa_divided_by_2 fsa_divided_by_3 bin_operation \
   fsa_openvpn fsa_number dfa_example dfa_example1 mindfa_example1 \
   dfa_abstar fsa_sep_23_star_openvpn

fsm_intro.ps fsm_intro.pdf fsm_intro.dvi: ${fsa_list:=.eps}

.ps.pdf:
	${PROG.${PS2PDF}} "$<" "$@"

#.tex.pdf:
#	pdflatex "$<" && pdflatex "$<"

.dot.eps:
	${PROG.${DOT}} -Tps ${.IMPSRC} > ${.TARGET}

.dvi.ps:
	${PROG.${DVIPS}} $<

.tex.dvi:
	${PROG.${LATEX}} "${.IMPSRC}" && ${PROG.${LATEX}} "${.IMPSRC}"

.gz:
	gzip ${.IMPSRC}

GARBAGE=	*.dvi *.aux *.vrb *.toc *.snm *.log *.nav *.out *.eps *.ps
CLEANFILES+=	${GARBAGE} *.pdf *.ps

.PHONY: clean-garbage
clean-garbage:
	rm -f ${GARBAGE}

.include <mkc.mk>
