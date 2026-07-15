#!/usr/bin/env python3
"""
Test WebEngine accessibility by opening the Help window via AT-SPI navigation
and verifying content is exposed through the accessibility tree.
"""

import gi
gi.require_version("Atspi", "2.0")
from gi.repository import Atspi
import subprocess
import time
import os
import sys
from pathlib import Path


JASP_BINARY = Path("/home/virtuoos/JASP-screenreader/jasp-desktop/build/Desktop/JASP")


def click_element(element):
    """Click an accessible element via its first action."""
    if element.get_n_actions() > 0:
        element.do_action(0)
        return True
    return False


def find_by_name(parent, name, role=None, timeout=5, recursive=True):
    """Search for an element by name (optionally by role) with retries."""
    name_lower = name.lower()
    for _ in range(timeout * 2):
        try:
            result = _search(parent, name_lower, role, recursive)
            if result:
                return result
        except Exception:
            pass
        time.sleep(0.5)
    return None


def _search(obj, name_lower, role, recursive):
    for i in range(obj.get_child_count()):
        try:
            child = obj.get_child_at_index(i)
            if name_lower in child.get_name().lower():
                if role is None or child.get_role_name().lower() == role.lower():
                    return child
            if role is not None and child.get_role_name().lower() == role.lower():
                if child.get_name().lower() == name_lower:
                    return child
            if recursive:
                result = _search(child, name_lower, role, recursive)
                if result:
                    return result
        except Exception:
            pass
    return None


def find_by_desc(parent, description, timeout=3):
    """Find element by accessible description."""
    desc_lower = description.lower()
    for _ in range(timeout * 2):
        for i in range(parent.get_child_count()):
            try:
                child = parent.get_child_at_index(i)
                if desc_lower in (child.get_description().lower() or ""):
                    return child
                # Also search children
                result = _search_desc(child, desc_lower)
                if result:
                    return result
            except Exception:
                pass
        time.sleep(0.5)
    return None


def _search_desc(obj, desc_lower):
    for i in range(obj.get_child_count()):
        try:
            child = obj.get_child_at_index(i)
            if desc_lower in (child.get_description().lower() or ""):
                return child
            result = _search_desc(child, desc_lower)
            if result:
                return result
        except Exception:
            pass
    return None


def dump_tree(obj, depth=0, max_depth=3):
    """Dump accessible tree for debugging."""
    if depth > max_depth:
        return
    try:
        n = obj.get_name()
        r = obj.get_role_name()
        d = getattr(obj, "get_description", lambda: "")()
        cc = obj.get_child_count()
        info = f"{'  ' * depth}{n!r} role={r}"
        if d:
            info += f" desc={d!r}"
        info += f" cc={cc}"
        print(info)
        for i in range(min(cc, 10)):
            try:
                child = obj.get_child_at_index(i)
                dump_tree(child, depth + 1, max_depth)
            except Exception:
                pass
    except Exception:
        print(f"{'  ' * depth}[error]")


def main():
    Atspi.init()

    # Find JASP app with main window
    print("Looking for JASP...")
    app = None
    main_window = None
    for attempt in range(30):
        time.sleep(1)
        try:
            desktop = Atspi.get_desktop(0)
            for i in range(desktop.get_child_count()):
                a = desktop.get_child_at_index(i)
                if "jasp" not in a.get_name().lower():
                    continue
                for j in range(a.get_child_count()):
                    try:
                        c = a.get_child_at_index(j)
                        if c.get_name() == "JASP" and c.get_role_name() == "frame" and c.get_child_count() > 50:
                            app = a
                            main_window = c
                            break
                    except Exception:
                        pass
                if main_window:
                    break
        except Exception:
            pass
        if main_window:
            break

    if not main_window:
        print("FATAL: Could not find JASP main window")
        sys.exit(1)
    print(f"Found main window: {main_window.get_child_count()} children")

    # Step 1: Click "Main menu" button
    print("Clicking 'Main menu' button...")
    hamburger_btn = find_by_name(main_window, "Main menu", "button", timeout=3)
    if not hamburger_btn:
        print("FATAL: 'Main menu' button not found")
        sys.exit(1)
    if not click_element(hamburger_btn):
        print("FATAL: Could not click 'Main menu'")
        sys.exit(1)
    time.sleep(2)

    # Step 2: Click "Preferences" button in FileMenu
    print("Looking for 'Preferences' button...")
    prefs_btn = find_by_name(app, "Preferences", "button", timeout=5)
    if not prefs_btn:
        print("FATAL: 'Preferences' button not found")
        sys.exit(1)
    if not click_element(prefs_btn):
        print("FATAL: Could not click 'Preferences'")
        sys.exit(1)
    time.sleep(2)

    # Step 3: Find Help button in Preferences page
    print("Looking for help info-button...")
    help_btn = None
    # Try by name first (our fix added text="Help")
    help_btn = find_by_name(app, "Help", "button", timeout=5)
    if not help_btn:
        # Try by description
        print("  Trying by description...")
        help_btn = find_by_desc(app, "Show info about", timeout=3)
    if not help_btn:
        print("FATAL: Help button not found")
        sys.exit(1)
    print(f"  Found help button: {help_btn.get_name()!r}")
    if not click_element(help_btn):
        print("FATAL: Could not click help button")
        sys.exit(1)
    time.sleep(3)

    # Step 4: Find Help window frame
    print("Looking for Help window...")
    help_frame = None
    for _ in range(15):
        time.sleep(1)
        try:
            desktop = Atspi.get_desktop(0)
            for i in range(desktop.get_child_count()):
                a = desktop.get_child_at_index(i)
                for j in range(a.get_child_count()):
                    try:
                        c = a.get_child_at_index(j)
                        if c.get_name() == "JASP Help" and c.get_role_name() == "frame":
                            help_frame = c
                            break
                    except Exception:
                        pass
                if help_frame:
                    break
        except Exception:
            pass
        if help_frame:
            break

    if not help_frame:
        print("FATAL: Help window not found")
        sys.exit(1)

    print(f"Help window found: {help_frame.get_child_count()} children")
    dump_tree(help_frame, max_depth=3)

    # Step 5: Verify document web
    doc = None
    for i in range(help_frame.get_child_count()):
        try:
            c = help_frame.get_child_at_index(i)
            if "document" in c.get_role_name().lower():
                doc = c
                break
        except Exception:
            pass

    if not doc:
        print("FAIL: No document web found in Help window")
        sys.exit(1)

    # The outer document web is Qt's accessible wrapper (may have no name).
    # The inner document web is the browser accessibility tree with content.
    doc_root = doc
    if doc.get_child_count() == 1:
        try:
            inner = doc.get_child_at_index(0)
            if "document" in inner.get_role_name().lower():
                doc_root = inner
        except Exception:
            pass

    print(f"\nDocument web: name={doc_root.get_name()!r} role={doc_root.get_role_name()} children={doc_root.get_child_count()}")

    if doc_root.get_name() != "JASP Help":
        print(f"FAIL: Document web name is {doc_root.get_name()!r}, expected 'JASP Help'")
        sys.exit(1)

    if doc_root.get_child_count() == 0:
        print("FAIL: Document web has no accessible children")
        sys.exit(1)

    print("PASS: Document web has accessible children!")
    print("\nDocument web children:")
    dump_tree(doc_root, max_depth=2)

    # Check for text content
    text_found = False
    for i in range(doc_root.get_child_count()):
        try:
            c = doc_root.get_child_at_index(i)
            if "text" in c.get_role_name().lower() or "heading" in c.get_role_name().lower():
                text_found = True
                print(f"  Found text: {c.get_name()!r} role={c.get_role_name()}")
                break
        except Exception:
            pass

    if not text_found:
        print("FAIL: No text/heading elements in document web")
        sys.exit(1)

    print("\nALL CHECKS PASSED!")


if __name__ == "__main__":
    main()
