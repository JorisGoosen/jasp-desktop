#!/usr/bin/env python3
"""
Shared utilities for JASP accessibility tests using AT-SPI2.
Import from this module to avoid duplicating tree-search, click,
and window-discovery logic across test files.
"""

import time
import sys
from pathlib import Path

try:
    gi = __import__("gi")
    gi.require_version("Atspi", "2.0")
    from gi.repository import Atspi
except ImportError as e:
    print(f"PyGObject not available: {e}")
    sys.exit(77)


def repo_root():
    """Absolute path to the JASP-screenreader repo root."""
    return Path(__file__).resolve().parent.parent.parent


def jasp_binary():
    """Absolute path to the locally built JASP binary."""
    return repo_root() / "jasp-desktop" / "build" / "Desktop" / "JASP"


def tool_script(name):
    """Absolute path to a script in the Tests/ directory."""
    return Path(__file__).resolve().parent / name


# ── tree search helpers ──────────────────────────────────────────────


def find_all(obj, depth=0, max_depth=8):
    """Return list of (role, name, child) tuples for all descendants."""
    elements = []
    if depth > max_depth:
        return elements
    try:
        cc = min(obj.get_child_count(), 100)
        for i in range(cc):
            child = obj.get_child_at_index(i)
            role = child.get_role_name() or "unknown"
            name = child.get_name() or ""
            elements.append((role, name, child))
            elements.extend(find_all(child, depth + 1, max_depth))
    except Exception:
        pass
    return elements


def find_all_by_role(parent, role, name_contains=None, depth=0, max_depth=8):
    """Return list of descendants matching a specific role."""
    results = []
    if depth > max_depth:
        return results
    try:
        cc = min(parent.get_child_count(), 100)
        for i in range(cc):
            child = parent.get_child_at_index(i)
            if child.get_role_name().lower() == role.lower():
                if name_contains is None or name_contains.lower() in child.get_name().lower():
                    results.append(child)
            results.extend(find_all_by_role(child, role, name_contains, depth + 1, max_depth))
    except Exception:
        pass
    return results
    return results


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


# ── interaction helpers ──────────────────────────────────────────────


def click_element(element):
    """Click an accessible element via its first click/press/activate action."""
    try:
        n_actions = element.get_n_actions()
    except AttributeError:
        try:
            n_actions = element.get_action_count()
        except AttributeError:
            try:
                action_iface = element.query_action()
                n_actions = action_iface.get_n_actions()
            except Exception:
                try:
                    element.do_action(0)
                    return True
                except Exception:
                    pass
                return False

    if n_actions == 0:
        return False

    for a in range(n_actions):
        try:
            aname = element.get_action_name(a).lower()
        except Exception:
            aname = ""
        if "click" in aname or "press" in aname or "activate" in aname or "toggle" in aname or n_actions == 1:
            try:
                element.do_action(a)
                return True
            except Exception:
                pass

    try:
        element.do_action(0)
        return True
    except Exception:
        pass

    return False


def get_jasp_app():
    """Get a fresh reference to the JASP application with the most children (main window app)."""
    try:
        desktop = Atspi.get_desktop(0)
        best = None
        best_cc = -1
        for i in range(desktop.get_child_count()):
            a = desktop.get_child_at_index(i)
            if "jasp" in a.get_name().lower():
                cc = a.get_child_count()
                if cc > best_cc:
                    best_cc = cc
                    best = a
        return best
    except Exception:
        pass
    return None


# ── app/window discovery ─────────────────────────────────────────────


def find_jasp_app(timeout=30, main_window_names=None):
    """
    Wait for JASP to start and return (app, main_window).
    JASP may register multiple applications on the AT-SPI bus;
    main_window_names is a set/tuple of acceptable frame names.
    Defaults to ('JASP',) for bare startup; pass ('JASP', 'Sleep') etc.
    when loading a .jasp file by name.
    If None, accepts any frame with > 3 children.
    """
    if main_window_names is None:
        main_window_names = ("JASP",)

    Atspi.init()
    app = None
    main_window = None
    for attempt in range(timeout):
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
                        if c.get_role_name() == "frame" and c.get_child_count() > 3:
                            if c.get_name() in main_window_names:
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
    return app, main_window


def find_document_web(app):
    """
    Find the most content-rich document web in the JASP app.
    JASP has multiple document web elements (help, about, community, results).
    Returns the one with the largest ancestor count.
    """
    all_docs = find_all_by_role(app, "document web")
    if not all_docs:
        return None

    best_doc = None
    best_total = 0
    for d in all_docs:
        total = d.get_child_count()
        for j in range(min(d.get_child_count(), 3)):
            try:
                cc = d.get_child_at_index(j)
                total += cc.get_child_count()
            except Exception:
                pass
        if total > best_total:
            best_total = total
            best_doc = d
    return best_doc


def dismiss_dialogs(app):
    """Click Close/OK buttons on any startup dialogs."""
    for i in range(app.get_child_count()):
        try:
            child = app.get_child_at_index(i)
            if child.get_role_name() != "frame":
                continue
            name = child.get_name()
            if name in ("JASP", "Data Preview"):
                continue
            for j in range(child.get_child_count()):
                try:
                    gc = child.get_child_at_index(j)
                    gc_name = gc.get_name().lower()
                    if gc_name in ("close", "continue", "ok", "accept", "no encryption", "skip"):
                        for a in range(gc.get_action_count()):
                            if "click" in gc.get_action_name(a).lower():
                                gc.do_action(a)
                                return
                except Exception:
                    pass
        except Exception:
            pass


def find_window_by_name(app, window_name, timeout=10):
    """Find a frame/window child of app by name, with retry."""
    wl = window_name.lower()
    for _ in range(timeout * 2):
        desktop = Atspi.get_desktop(0)
        for i in range(desktop.get_child_count()):
            try:
                a = desktop.get_child_at_index(i)
                for j in range(a.get_child_count()):
                    try:
                        c = a.get_child_at_index(j)
                        if c.get_role_name() in ("frame", "window") and wl in c.get_name().lower():
                            return c
                    except Exception:
                        pass
            except Exception:
                pass
        time.sleep(0.5)
    return None


def find_all_buttons(parent, max_depth=8):
    """Return list of (name, element) tuples for all button-role descendants."""
    results = []
    elements = find_all(parent, max_depth=max_depth)
    for role, name, child in elements:
        if "button" in role.lower() and not child.get_name().startswith(("Tool",)):
            results.append((name, child))
    return results


def find_menu_items(parent):
    """Return list of (name, element) for all menu-item-role descendants."""
    results = []
    elements = find_all(parent, max_depth=8)
    for role, name, child in elements:
        if "menu item" in role.lower():
            results.append((name, child))
    return results


def generate_key_event(keyval):
    """Send a key event via AT-SPI to generate keyboard events."""
    try:
        Atspi.generate_keyboard_event(keyval, "", Atspi.KeySynchType.NO_SYNCH | Atspi.KeySynchType.NO_FILTERING)
        return True
    except Exception:
        return False


def type_text(text, delay=0.01):
    """Type a string character-by-character via AT-SPI keyboard events."""
    for ch in text:
        generate_key_event(ord(ch))
        time.sleep(delay)


def close_menu():
    """Send Escape key to close any open menus/dialogs."""
    for _ in range(3):
        generate_key_event(9)  # Escape
        time.sleep(0.3)


def ensure_menu_closed(app, main_window):
    """Press Escape until the accessible tree returns to baseline size."""
    try:
        close_menu()
        time.sleep(0.5)
        dismiss_dialogs(app)
        time.sleep(0.5)
    except Exception:
        pass


def wait_for_element(parent, name, role="button", timeout=10):
    """Wait for an element by name and role to appear, with retries."""
    for _ in range(timeout * 2):
        result = find_by_name(parent, name, role, timeout=1)
        if result:
            return result
        time.sleep(0.5)
    return None


def count_tree_elements(obj, max_depth=8):
    """Count total accessible elements in a subtree."""
    elements = find_all(obj, max_depth=max_depth)
    return len(elements)


# ── debug helpers ────────────────────────────────────────────────────


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