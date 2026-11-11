#!/usr/bin/env python3
"""Build the 50-chapter Tim Rules sequel as a trade-paperback PDF."""

from pathlib import Path
import re
import sys

from reportlab.lib.units import mm
from reportlab.platypus import (
    Frame,
    NextPageTemplate,
    PageBreak,
    PageTemplate,
    Paragraph,
    Spacer,
)
from reportlab.platypus.tableofcontents import TableOfContents
from reportlab.lib.styles import ParagraphStyle

sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_tim_rules_book import (  # noqa: E402
    BookDocTemplate,
    INK,
    PAGE_SIZE,
    chapter_flowables,
    header_footer,
    make_styles,
)

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Tim_Rules_2_Save_the_School_Manuscript.md"
OUTPUT = ROOT / "Tim_Rules_2_Save_the_School.pdf"


def read_chapters(source=SOURCE):
    text = source.read_text(encoding="utf-8")
    matches = list(re.finditer(r"^## Chapter (\d+): (.+)$", text, re.MULTILINE))
    chapters = []
    for index, match in enumerate(matches):
        end = matches[index + 1].start() if index + 1 < len(matches) else len(text)
        body = text[match.end() : end].strip().splitlines()
        chapters.append(
            {"number": int(match.group(1)), "title": match.group(2).strip(), "body": body}
        )
    assert [chapter["number"] for chapter in chapters] == list(range(1, 51))
    return chapters


def build(output=OUTPUT):
    chapters = read_chapters()
    styles = make_styles()
    width, height = PAGE_SIZE
    doc = BookDocTemplate(
        str(output),
        pagesize=PAGE_SIZE,
        leftMargin=19 * mm,
        rightMargin=19 * mm,
        topMargin=21 * mm,
        bottomMargin=21 * mm,
        title="Tim Rules 2 — Save the School",
        author="",
        subject="The complete 50-chapter sequel to Tim Rules",
        creator="Tim Rules manuscript compiler",
    )
    frame = Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="book")
    doc.addPageTemplates(
        [
            PageTemplate(
                id="Front",
                frames=[frame],
                onPage=lambda canvas, document: header_footer(canvas, document, False),
            ),
            PageTemplate(
                id="Body",
                frames=[frame],
                onPage=lambda canvas, document: header_footer(canvas, document, True),
            ),
        ]
    )
    story = [
        Spacer(1, height * 0.21),
        Paragraph("TIM RULES 2", styles["BookTitle"]),
        Spacer(1, 5),
        Paragraph("Save the School", styles["BookSubtitle"]),
        Spacer(1, 10),
        Paragraph("The Complete Sequel — 50 Chapters", styles["BookSubtitle"]),
        NextPageTemplate("Front"),
        PageBreak(),
        Paragraph("Contents", styles["ContentsTitle"]),
    ]
    toc = TableOfContents()
    toc.levelStyles = [
        ParagraphStyle(
            name="TOCChapter",
            fontName="Times-Roman",
            fontSize=9,
            leading=13,
            leftIndent=0,
            firstLineIndent=0,
            rightIndent=12,
            textColor=INK,
        )
    ]
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
