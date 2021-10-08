\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{iamthesis}[2014/08/16 Example LaTeX class]

\LoadClass[10pt, fleqn, b5paper]{book}
\ProcessOptions\relax

%----------------------------------------------------------------%
%   Set language
%----------------------------------------------------------------%
\RequirePackage{babel}
\DeclareOption{english}{
\PassOptionsToPackage{english}{babel}
\def\slovak{0}
}

\DeclareOption{slovak}{
%  \RequirePackage{babel}
\PassOptionsToPackage{slovak}{babel}
\def\slovak{1}

}

\ExecuteOptions{english} % default is english

\ProcessOptions\relax

\if\slovak1
\RequirePackage{icomma}
\fi
%----------------------------------------------------------------%
%   Typeset packages
%----------------------------------------------------------------%
\RequirePackage{lmodern}
\RequirePackage[T1]{fontenc}
\RequirePackage[utf8]{inputenc}
\RequirePackage{microtype}
\RequirePackage{eurosym}


%----------------------------------------------------------------%
% MATH                                                           %
\RequirePackage{amssymb}
\RequirePackage{amsmath}
\RequirePackage{amsfonts} 
\RequirePackage{bm}
\RequirePackage{mathtools}
\RequirePackage{dcolumn}
\RequirePackage{nicefrac}
\RequirePackage{siunitx}

%----------------------------------------------------------------%

%----------------------------------------------------------------%
% Figures, tables                                                %
\RequirePackage{graphicx}
\RequirePackage{float}
\RequirePackage{multicol}
\RequirePackage{multirow}
\RequirePackage{array}
\RequirePackage{xcolor}
\RequirePackage{wrapfig}
\RequirePackage{placeins}
\RequirePackage{bbm}
\RequirePackage[hang,bf]{caption}
\RequirePackage{etoolbox}
\RequirePackage{tabularx}
\RequirePackage{paralist}
\RequirePackage{subfigure}
%----------------------------------------------------------------%

%----------------------------------------------------------------%
% Other packages                                                 %
\RequirePackage{fancyhdr}
\RequirePackage{lastpage}
\RequirePackage{pdfpages}
\RequirePackage{natbib}
\RequirePackage{nicefrac}
\RequirePackage{amsthm}
\RequirePackage{thmtools}
\RequirePackage{needspace}
\RequirePackage{etoolbox}
\RequirePackage[b5paper]{geometry}
\setcounter{secnumdepth}{3}
\RequirePackage{booktabs}
\RequirePackage{url}
\RequirePackage{titlesec}
\RequirePackage{enumitem}
\titlespacing{\section}{0pt}{\parskip+6pt}{-\parskip+6pt}
\titlespacing{\subsection}{0pt}{\parskip}{-\parskip+4pt}
%----------------------------------------------------------------%

%----------------------------------------------------------------%
%   HYPERLINKS                                                   %
%   http://en.wikibooks.org/wiki/LaTeX/Hyperlinks                %
%----------------------------------------------------------------%
\usepackage[bookmarks=true,colorlinks=false,linkcolor=dark_blue,
pdfstartview={FitV},citecolor=ddark_green,linktoc=all,pdfauthor={IAM thesis 
template},urlcolor=blue]{hyperref}

%----------------------------------------------------------------%
%    PAGE GEOMETRY
%----------------------------------------------------------------%
\addtolength{\parskip}{\baselineskip}
\setlength{\parindent}{0pt}
\setlength{\paperwidth}{176mm}  %b5
\setlength{\paperheight}{250mm} %b5
\setlength{\textheight}{185mm} % 540pt = 7.5inch = 190.5mm
\setlength{\topmargin}{-5.4mm}
\setlength{\headheight}{8mm}
\setlength{\headsep}{7mm}
\setlength{\footskip}{10.0mm}
\setlength{\textwidth}{121.36mm}  % 129.36 mm  % width and margins must add to 
\addtolength{\textwidth}{26pt}
%%176 mm:
\setlength{\oddsidemargin}{0pt} % 1 inch + 1.24 mm = 26.64 mm inside
\setlength{\evensidemargin}{-15pt} % 1 inch - 5.4 mm = 20 mm outside

%----------------------------------------------------------------%
%   HEADINGS SETUP
%----------------------------------------------------------------%
\pagestyle{fancyplain}
\renewcommand{\chaptermark}[1]{\markboth{#1}{}}
\renewcommand{\sectionmark}[1]{\markright{\thesection\ #1}}
\lhead[\fancyplain{}{\sffamily\bfseries\thepage}]%
{\fancyplain{}{\sffamily\bfseries\rightmark}}
\rhead[\fancyplain{}{\sffamily\bfseries\leftmark}]%
{\fancyplain{}{\sffamily\bfseries\thepage}}
\cfoot{}



%----------------------------------------------------------------%
%   CHAPTER AND APPENDIX STYLING SETUP
%----------------------------------------------------------------%
\def\@makechapterhead#1{%
  \vspace*{15\p@}%
  {\parindent \z@ \raggedright \normalfont
  \ifnum \c@secnumdepth >\m@ne
  \if@mainmatter
  \large\scshape \@chapapp \space\space \huge\upshape\thechapter
  \par\nobreak
  \vskip 20\p@
  \fi
  \fi
  \interlinepenalty\@M
  \flushleft\parbox{\textwidth}{\raggedleft \sffamily\huge\bfseries  #1}
  \par\nobreak
  \vskip 15pt
  \hrule height 0.5pt
  \vskip 10\p@
  }}
  
  
\newcommand{\ifstringequal}[4]{%
\ifnum\pdfstrcmp{#1}{#2}=0
  #3%
  \else
  #4%
  \fi
  }

%----------------------------------------------------------------%
%   Typeset the coverpage
%----------------------------------------------------------------%
\newcommand{\difftopmargin}{0pt}
\newcommand{\diffleftmargin}{30pt}

\newcommand{\coverpage}{
\thispagestyle{empty}

\begin{center}
\if\slovak1
\textbf{\large SLOVENSKÁ TECHNICKÁ UNIVERZITA V~BRATISLAVE}

\textbf{\large FAKULTA CHEMICKEJ A~POTRAVINÁRSKEJ TECHNOLÓGIE}
\else
\textbf{\large SLOVAK UNIVERSITY OF TECHNOLOGY IN~BRATISLAVA}

\textbf{\large FACULTY OF CHEMICAL AND FOOD TECHNOLOGY}
\fi
\end{center}
\ifstringequal{\thesislevel}{min}{
	%
}{%
\if\slovak1
Evidenčné číslo: FCHPT-\thesisnumber
\else
Reg. No.: FCHPT-\thesisnumber
\fi
}
\begin{center}
\vspace{100pt}
\textbf{\huge \thesistitle}

\vspace{10pt}
\ifstringequal{\thesislevel}{min}{
\if\slovak1
\textbf{Písomná práca k dizertačnej skúške}
\else
\textbf{Minimal thesis}
\fi
}{%
\if\slovak1
\textbf{\thesistype~PRÁCA}
\else
\textbf{\thesistype~THESIS}
\fi
}
\end{center}
\vfill

\textbf{\large \thesisyear} \hfill \textbf{\large \thesisauthor}


\newpage
\thispagestyle{empty}
\phantom{M}

\newpage
\thispagestyle{empty}

\begin{center}
\if\slovak1
\textbf{\large SLOVENSKÁ TECHNICKÁ UNIVERZITA V~BRATISLAVE}

\textbf{\large FAKULTA CHEMICKEJ A~POTRAVINÁRSKEJ TECHNOLÓGIE}
\else
\textbf{\large SLOVAK UNIVERSITY OF TECHNOLOGY IN~BRATISLAVA}

\textbf{\large FACULTY OF CHEMICAL AND FOOD TECHNOLOGY}
\fi
\end{center}
\ifstringequal{\thesislevel}{min}{
	%
}{%
\if\slovak1
Evidenčné číslo: FCHPT-\thesisnumber
\else
Reg. No.: FCHPT-\thesisnumber
\fi
}

\begin{center}
\vspace{100pt}
\textbf{\huge \thesistitle}

\vspace{10pt}	
\ifstringequal{\thesislevel}{min}{
\if\slovak1
\textbf{Písomná práca k dizertačnej skúške}
\else
\textbf{Minimal thesis}
\fi
}{%
\if\slovak1
\textbf{\thesistype~PRÁCA}
\else
\textbf{\thesistype~THESIS}
\fi
}
\end{center}	
\vfill	

\begin{small} 
\if\slovak1
\begin{tabular}{ll}
Študijný program:     & Riadenie Procesov    
\\
Študijný odbor:         & Kybernetika \\
Školiace pracovisko:        &\thesisinst   
\\
Vedúci práce:   &\thesissupervisor \\
\end{tabular}	
\else
\begin{tabular}{ll}
Study programme:     & Process Control    \\
Study field:         & Cybernetics \\
Training workspace:        &\thesisinst   \\
Thesis supervisor:   & \thesissupervisor \\
\end{tabular}	
\fi		
\end{small}
\vfill
\textbf{\large \thesisyear} \hfill \textbf{\large \thesisauthor}
\newpage
\thispagestyle{empty}
\phantom{M}
}

\newcommand{\providebibliography}{
%	\renewcommand{\sectionmark}[1]{\markright{#1}}
\if\slovak1
\sectionmark{Literatúra}
\addcontentsline{toc}{chapter}{Literatúra}
\else
\sectionmark{Bibliography}
\addcontentsline{toc}{chapter}{Bibliography}
\fi
\bibliographystyle{plain}
}

%----------------------------------------------------------------%
%   Define macros
%----------------------------------------------------------------%
% upright sub-index
\newcommand{\ui}[2]{#1 _{\text{#2}}}
% upright sub-index with variable
\newcommand{\uis}[3]{#1 _{\text{#2}, #3}}
% sampling time
\newcommand{\Ts}{\ui{T}{s}}
% left-right parenthesis
\newcommand{\lrp}[1]{\ensuremath{\left( #1 \right)}}
% left-right brackets
\newcommand{\lrb}[1]{\ensuremath{\left[ #1 \right]}}

\newcommand{\dif}[2]{\ensuremath{\frac{\mathrm{d}\, #1}{\mathrm{d}\, #2}}}
\newcommand{\diff}[2]{\ensuremath{\frac{\partial\, #1}{\partial\, #2}}}
\newcommand{\diffxy}[3]{\ensuremath{\frac{\partial^2\, #1}{\partial\,
#2\; \partial\, #3}}}
\newcommand{\diffat}[3]{\ensuremath{\left.\frac{\partial #1}{\partial 
#2}\right\vert_{#3}}}
\newcommand{\difat}[3]{\ensuremath{\left.\frac{\mathrm{d} #1}{\mathrm{d} 
#2}\right\vert_{#3}}}
\newcommand{\dt}[1]{\ensuremath{\frac{\mathrm{d}#1}{\mathrm{d}t}}}
%\newcommand{\dtat}[1]{\ensuremath{\frac{\mathrm{d}#1}{\mathrm{d}t}}}
%----------------------------------------------------------------%
%   end of file
%----------------------------------------------------------------%