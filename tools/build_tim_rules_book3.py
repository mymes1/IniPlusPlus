#!/usr/bin/env python3
"""Build Tim Rules 3: The Open Door in the Books 1 and 2 text layout."""

from pathlib import Path
import re
import sys

from reportlab.lib.enums import TA_CENTER
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.platypus import Frame, NextPageTemplate, PageBreak, PageTemplate, Paragraph, Spacer
from reportlab.platypus.tableofcontents import TableOfContents

sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_tim_rules_book import BookDocTemplate, INK, PAGE_SIZE, chapter_flowables, header_footer, make_styles

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Tim_Rules_3_The_Open_Door_Manuscript.md"
OUTPUT = ROOT / "Tim_Rules_3_The_Open_Door.pdf"


def read_chapters(source=SOURCE):
    text = source.read_text(encoding="utf-8")
    matches = list(re.finditer(r"^## Chapter (\d+): (.+)$", text, re.MULTILINE))
    chapters = []
    for index, match in enumerate(matches):
        end = matches[index + 1].start() if index + 1 < len(matches) else len(text)
        chapters.append({
            "number": int(match.group(1)),
            "title": match.group(2).strip(),
            "body": text[match.end():end].strip().splitlines(),
        })
    assert [c["number"] for c in chapters] == list(range(1, 51))
    return chapters


def build(output=OUTPUT):
    chapters = read_chapters()
    styles = make_styles()
    # Match the body text and layout of Books 1 and 2 exactly.
    styles["Story"].fontSize = 10.5
    styles["Story"].leading = 14.2
    styles["Story"].spaceAfter = 2.5
    styles["Section"].fontSize = 8.5
    styles["Section"].leading = 12
    styles["Section"].spaceBefore = 13
    styles["Section"].spaceAfter = 9
    styles["ChapterTitle"].fontSize = 20
    styles["ChapterTitle"].leading = 25
    styles["BookTitle"].fontSize = 35
    styles["BookSubtitle"].fontSize = 12
    styles["BookSubtitle"].leading = 17
    width, height = PAGE_SIZE
    doc = BookDocTemplate(
        str(output), pagesize=PAGE_SIZE,
        leftMargin=19*mm, rightMargin=19*mm, topMargin=21*mm, bottomMargin=21*mm,
        title="Tim Rules 3 — The Open Door", author="",
        subject="The complete 50-chapter third Tim Rules book",
        creator="Tim Rules manuscript compiler",
    )
    frame = Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="book")
    doc.addPageTemplates([
        PageTemplate(id="Front", frames=[frame], onPage=lambda c,d: header_footer(c,d,False)),
        PageTemplate(id="Body", frames=[frame], onPage=lambda c,d: header_footer(c,d,True)),
    ])
    story = [
        Spacer(1, height*0.19), Paragraph("TIM RULES 3", styles["BookTitle"]),
        Spacer(1, 7), Paragraph("The Open Door", styles["BookSubtitle"]),
        Spacer(1, 12), Paragraph("The Complete Third Book — 50 Chapters", styles["BookSubtitle"]),
        NextPageTemplate("Front"), PageBreak(), Paragraph("Contents", styles["ContentsTitle"]),
    ]
    toc = TableOfContents()
    toc.levelStyles = [ParagraphStyle(
        name="TOCChapter", fontName="Times-Roman", fontSize=9, leading=13,
        leftIndent=0, firstLineIndent=0, rightIndent=12, textColor=INK,
    )]
    toc.dotsMinLevel = 0
    story.extend([toc, NextPageTemplate("Body"), PageBreak()])
    for index, chapter in enumerate(chapters):
        if index:
            story.append(PageBreak())
        story.extend(chapter_flowables(chapter, styles))
    doc.multiBuild(story)
    print(f"Built {output} with {len(chapters)} chapters.")


if __name__ == "__main__":
    build()
