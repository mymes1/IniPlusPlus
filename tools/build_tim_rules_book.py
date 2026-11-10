#!/usr/bin/env python3
"""Extract the complete Tim Rules chapters from the chat transcript and build a book PDF."""

from __future__ import annotations

import argparse
import re
from pathlib import Path
from xml.sax.saxutils import escape

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_LEFT, TA_RIGHT
from reportlab.lib.pagesizes import inch
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.platypus import (
    BaseDocTemplate,
    Frame,
    KeepTogether,
    NextPageTemplate,
    PageBreak,
    PageTemplate,
    Paragraph,
    Spacer,
)
from reportlab.platypus.tableofcontents import TableOfContents

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Write_Tim_Rules_Chapter_1.txt"
MANUSCRIPT = ROOT / "Tim_Rules_Complete_Manuscript.md"
OUTPUT = ROOT / "Tim_Rules_Complete_Book.pdf"
PAGE_SIZE = (6 * inch, 9 * inch)  # Standard trade-paperback proportions.
INK = colors.HexColor("#20242A")
ACCENT = colors.HexColor("#9C3D35")
MUTED = colors.HexColor("#6C6A67")
PAPER_RULE = colors.HexColor("#C9BBA8")


def extract_chapters(source: Path) -> list[dict[str, object]]:
    """Keep the first complete occurrence of each numbered chapter.

    The transcript contains an unfinished replacement for Chapter 2. Selecting the
    first occurrence retains its complete original text rather than that fragment.
    """
    lines = source.read_text(encoding="utf-8-sig").splitlines()
    heading = re.compile(r"^Chapter\s+(\d+):\s*(.+?)\s*$")
    chapters: dict[int, dict[str, object]] = {}

    for index, line in enumerate(lines):
        match = heading.match(line.strip())
        if not match:
            continue
        number = int(match.group(1))
        if number in chapters:
            continue
        body: list[str] = []
        for candidate in lines[index + 1 :]:
            if heading.match(candidate.strip()):
                break
            if candidate.startswith(":::") or candidate.strip() in {"You:", "ChatGPT:"}:
                break
            # ChatGPT's writing blocks often close with ":::" attached to the
            # chapter's final punctuation rather than on a separate line.
            if candidate.rstrip().endswith(":::"):
                body.append(candidate.rstrip()[:-3].rstrip())
                break
            body.append(candidate.rstrip())
        while body and not body[-1]:
            body.pop()
        chapters[number] = {
            "number": number,
            "title": match.group(2).strip(),
            "body": body,
        }

    missing = [number for number in range(1, 76) if number not in chapters]
    if missing:
        raise ValueError(f"Missing chapters: {missing}")
    result = [chapters[number] for number in range(1, 76)]
    if any(not chapter["body"] for chapter in result):
        raise ValueError("At least one extracted chapter is empty")
    return result


def write_manuscript(chapters: list[dict[str, object]], destination: Path) -> None:
    parts = ["# TIM RULES", "", "*The Complete Story — Chapters 1–75*", ""]
    for chapter in chapters:
        parts.extend(
            [
                f"## Chapter {chapter['number']}: {chapter['title']}",
                "",
                *chapter["body"],
                "",
            ]
        )
    destination.write_text("\n".join(parts).rstrip() + "\n", encoding="utf-8")


class BookDocTemplate(BaseDocTemplate):
    """Adds PDF outline entries and clickable table-of-contents entries."""

    def afterFlowable(self, flowable):
        if isinstance(flowable, Paragraph):
            style = flowable.style.name
            if style == "ChapterTitle":
                number = flowable.chapter_number
                title = flowable.chapter_title
                label = f"Chapter {number}"
                key = f"chapter-{number}"
                self.canv.bookmarkPage(key)
                self.canv.addOutlineEntry(
                    f"{label}: {title}", key, level=0, closed=False
                )
                self.notify("TOCEntry", (0, f"{label}: {title}", self.page - 1, key))


def make_styles():
    styles = getSampleStyleSheet()
    styles.add(
        ParagraphStyle(
            name="BookTitle",
            parent=styles["Title"],
            fontName="Times-Bold",
            fontSize=35,
            leading=39,
            alignment=TA_CENTER,
            textColor=INK,
            spaceAfter=12,
        )
    )
    styles.add(
        ParagraphStyle(
            name="BookSubtitle",
            parent=styles["Normal"],
            fontName="Times-Italic",
            fontSize=12,
            leading=17,
            alignment=TA_CENTER,
            textColor=MUTED,
        )
    )
    styles.add(
        ParagraphStyle(
            name="ContentsTitle",
            parent=styles["Heading1"],
            fontName="Times-Bold",
            fontSize=25,
            leading=29,
            alignment=TA_CENTER,
            textColor=INK,
            spaceAfter=20,
        )
    )
    styles.add(
        ParagraphStyle(
            name="ChapterTitle",
            parent=styles["Heading1"],
            fontName="Times-Bold",
            fontSize=20,
            leading=25,
            alignment=TA_CENTER,
            textColor=INK,
            spaceBefore=12,
            spaceAfter=22,
            keepWithNext=True,
        )
    )
    styles.add(
        ParagraphStyle(
            name="Story",
            parent=styles["BodyText"],
            fontName="Times-Roman",
            fontSize=10.5,
            leading=14.2,
            textColor=INK,
            alignment=TA_LEFT,
            spaceAfter=2.5,
            allowWidows=0,
            allowOrphans=0,
        )
    )
    styles.add(
        ParagraphStyle(
            name="Section",
            parent=styles["Heading2"],
            fontName="Helvetica-Bold",
            fontSize=8.5,
            leading=12,
            alignment=TA_CENTER,
            textColor=ACCENT,
            spaceBefore=13,
            spaceAfter=9,
            keepWithNext=True,
        )
    )
    styles.add(
        ParagraphStyle(
            name="StoryBreak",
            parent=styles["Normal"],
            fontSize=3,
            leading=5,
            spaceAfter=2,
        )
    )
    return styles


def header_footer(canvas, doc, body: bool):
    canvas.saveState()
    width, height = PAGE_SIZE
    if doc.page > 1:
        canvas.setStrokeColor(PAPER_RULE)
        canvas.setLineWidth(0.35)
        canvas.line(19 * mm, 15.2 * mm, width - 19 * mm, 15.2 * mm)
        canvas.setFillColor(MUTED)
        canvas.setFont("Times-Roman", 8.5)
        canvas.drawCentredString(width / 2, 10.8 * mm, str(doc.page - 1))
        if body:
            canvas.setFont("Times-Roman", 8)
            if doc.page % 2:
                canvas.drawString(19 * mm, height - 14 * mm, "TIM RULES")
            else:
                canvas.drawRightString(width - 19 * mm, height - 14 * mm, "TIM RULES")
    canvas.restoreState()


def chapter_flowables(chapter: dict[str, object], styles) -> list:
    number = chapter["number"]
    title = escape(str(chapter["title"]))
    heading = Paragraph(
        f'<font name="Helvetica-Bold" size="9" color="#9C3D35">CHAPTER {number}</font>'
        f"<br/>{title}",
        styles["ChapterTitle"],
    )
    heading.chapter_number = number
    heading.chapter_title = chapter["title"]
    story: list = [heading]
    previous_blank = True
    body = chapter["body"]
    for index, raw_line in enumerate(body):
        line = raw_line.strip()
        if not line:
            if story and not previous_blank:
                story.append(Spacer(1, 5))
            previous_blank = True
            continue
        next_blank = index + 1 == len(body) or not body[index + 1].strip()
        letters = "".join(character for character in line if character.isalpha())
        is_section = (
            previous_blank
            and next_blank
            and bool(letters)
            and line == line.upper()
            and len(line) <= 48
            and not line.endswith(("!", "?", '"', "."))
        )
        if is_section:
            # Remove excess space before a section heading, then keep the heading
            # attached to the first line of its scene.
            if story and isinstance(story[-1], Spacer):
                story.pop()
            story.append(Paragraph(escape(line), styles["Section"]))
        else:
            story.append(Paragraph(escape(line), styles["Story"]))
        previous_blank = False
    if number == 75:
        # Keep the closing thought and “THE END” together instead of leaving
        # the final two words by themselves on an otherwise empty page.
        story = story[:-8] + [KeepTogether(story[-8:])]
    return story


def build_pdf(chapters: list[dict[str, object]], destination: Path) -> None:
    styles = make_styles()
    width, height = PAGE_SIZE
    doc = BookDocTemplate(
        str(destination),
        pagesize=PAGE_SIZE,
        leftMargin=19 * mm,
        rightMargin=19 * mm,
        topMargin=21 * mm,
        bottomMargin=21 * mm,
        title="Tim Rules — The Complete Story",
        author="",
        subject="All 75 complete chapters extracted from the original conversation",
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
        Spacer(1, height * 0.25),
        Paragraph("TIM RULES", styles["BookTitle"]),
        Spacer(1, 5),
        Paragraph("The Complete Story", styles["BookSubtitle"]),
        Spacer(1, 10),
        Paragraph("Chapters 1–75", styles["BookSubtitle"]),
        NextPageTemplate("Front"),
        PageBreak(),
        Paragraph("Contents", styles["ContentsTitle"]),
    ]
    toc = TableOfContents()
    toc.levelStyles = [
        ParagraphStyle(
            name="TOCChapter",
            fontName="Times-Roman",
            fontSize=8.4,
            leading=11.6,
            leftIndent=0,
            firstLineIndent=0,
            rightIndent=12,
            textColor=INK,
            spaceBefore=0,
        )
    ]
    toc.dotsMinLevel = 0
    story.extend([toc, NextPageTemplate("Body"), PageBreak()])
    for index, chapter in enumerate(chapters):
        if index:
            story.append(PageBreak())
        story.extend(chapter_flowables(chapter, styles))
    doc.multiBuild(story)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=SOURCE)
    parser.add_argument("--output", type=Path, default=OUTPUT)
    parser.add_argument("--manuscript", type=Path, default=MANUSCRIPT)
    args = parser.parse_args()
    chapters = extract_chapters(args.source)
    write_manuscript(chapters, args.manuscript)
    build_pdf(chapters, args.output)
    words = sum(len(" ".join(chapter["body"]).split()) for chapter in chapters)
    print(f"Extracted {len(chapters)} chapters ({words:,} words).")
    print(f"Manuscript: {args.manuscript}")
    print(f"Book: {args.output}")


if __name__ == "__main__":
    main()
