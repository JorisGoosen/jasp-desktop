#!/usr/bin/env python3
"""
Check JASP results window accessibility for Sleep.jasp Descriptives output.
Assumes JASP is already running with Sleep.jasp loaded.
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


def find_all(obj, depth=0, max_depth=5):
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


def find_all_by_role(parent, role, name_contains=None, depth=0, max_depth=5):
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


def click(btn):
    try:
        n_actions = btn.get_n_actions()
    except AttributeError:
        try:
            n_actions = btn.get_action_count()
        except AttributeError:
            # Try the DBus action interface directly
            try:
                action_iface = btn.query_action()
                n_actions = action_iface.get_n_actions()
            except Exception:
                print(f"  Warning: Cannot get actions for '{btn.get_name()}', trying component click")
                # Fallback: try to simulate a click via the component interface
                try:
                    comp = btn.query_component()
                    # Just try do_action(0) blindly
                    btn.do_action(0)
                    return True
                except Exception:
                    pass
                return False
    for a in range(n_actions):
        try:
            name = btn.get_action_name(a).lower()
        except Exception:
            name = ""
        if "click" in name or "press" in name or n_actions == 1:
            try:
                btn.do_action(a)
                return True
            except Exception:
                pass
    return False


def main():
    Atspi.init()

    # Find JASP app and its main window
    app = None
    main_window = None
    for attempt in range(30):
        try:
            desktop = Atspi.get_desktop(0)
            if attempt == 0:
                print(f"Desktop has {desktop.get_child_count()} children")
            for i in range(desktop.get_child_count()):
                a = desktop.get_child_at_index(i)
                if attempt == 0:
                    print(f"  app[{i}]: name='{a.get_name()}', role='{a.get_role_name()}', children={a.get_child_count()}")
                    for j in range(a.get_child_count()):
                        try:
                            c = a.get_child_at_index(j)
                            print(f"    child[{j}]: name='{c.get_name()}', role='{c.get_role_name()}', children={c.get_child_count()}")
                        except Exception:
                            pass
                if "jasp" not in a.get_name().lower():
                    continue
                for j in range(a.get_child_count()):
                    try:
                        c = a.get_child_at_index(j)
                        if c.get_role_name() == "frame" and c.get_child_count() > 50:
                            app = a
                            main_window = c
                            print(f"Found main window: name='{c.get_name()}', children={c.get_child_count()}")
                            break
                    except Exception:
                        pass
                if main_window:
                    break
        except Exception as e:
            print(f"Error checking desktop: {e}")
        if main_window:
            break
        print(f"  Waiting for JASP main window... ({attempt+1}s)")
        time.sleep(1)

    if not main_window:
        print("FAIL: JASP main window not found via AT-SPI2")
        sys.exit(1)

    print(f"JASP found, main window has {main_window.get_child_count()} children")

    # Skip running analysis - Sleep.jasp auto-loads saved analyses
    # Just wait for results to render
    print("\n--- Waiting for results to render ---")
    time.sleep(8)

    # --- Check document web ---
    print("\n--- Looking for document web ---")
    # Print all document-like children in the app
    for i in range(app.get_child_count()):
        try:
            c = app.get_child_at_index(i)
            if "document" in c.get_role_name().lower():
                print(f"  Frame[{i}] '{c.get_name()}' role='{c.get_role_name()}' children={c.get_child_count()}")
                if c.get_child_count() > 0:
                    for j in range(min(c.get_child_count(), 5)):
                        try:
                            cc = c.get_child_at_index(j)
                            print(f"    child[{j}]: '{cc.get_name()}' role='{cc.get_role_name()}'")
                        except Exception:
                            pass
        except Exception:
            pass

    doc = None
    for i in range(app.get_child_count()):
        try:
            c = app.get_child_at_index(i)
            if "document" in c.get_role_name().lower() and "web" in c.get_role_name().lower():
                doc = c
                break
        except Exception:
            pass

    if not doc:
        all_docs = find_all_by_role(app, "document web")
        print(f"  Found {len(all_docs)} document web elements total")
        best_doc = None
        best_total = 0
        for idx, d in enumerate(all_docs):
            total_descendants = d.get_child_count()
            for j in range(min(d.get_child_count(), 3)):
                try:
                    cc = d.get_child_at_index(j)
                    total_descendants += cc.get_child_count()
                except Exception:
                    pass
            print(f"    doc[{idx}]: '{d.get_name()}' children={d.get_child_count()} total_descendants~{total_descendants}")
            if d.get_child_count() > 0:
                for j in range(min(d.get_child_count(), 3)):
                    try:
                        cc = d.get_child_at_index(j)
                        print(f"      child[{j}]: '{cc.get_name()}' role='{cc.get_role_name()}' children={cc.get_child_count()}")
                    except Exception:
                        pass
            if total_descendants > best_total:
                best_total = total_descendants
                best_doc = d
        if best_doc:
            doc = best_doc

    if not doc:
        print("FAIL: document web not found")
        sys.exit(1)

    print(f"  Document: '{doc.get_name()}', {doc.get_child_count()} children")

    doc_elements = find_all(doc, max_depth=6)

    desc_stats_found = False
    boxplots_found = False
    tables = []
    sections = []
    images = []

    for role, name, child in doc_elements:
        nl = name.lower()
        if "descriptive statistics" in nl:
            desc_stats_found = True
        if "boxplots" in nl or "box plot" in nl:
            boxplots_found = True
        if "table" in role.lower() and name:
            tables.append((role, name))
        if "section" in role.lower() and name:
            sections.append((role, name))
        if "image" in role.lower() or "graphic" in role.lower():
            images.append((role, name))

    print(f"\n  Total document elements: {len(doc_elements)}")
    print(f"\n  Sections ({len(sections)}):")
    for r, n in sections:
        print(f"    {r}: \"{n}\"")

    print(f"\n  Tables ({len(tables)}):")
    for r, n in tables:
        print(f"    {r}: \"{n}\"")

    print(f"\n  Images/Graphics ({len(images)}):")
    for r, n in images:
        print(f"    {r}: \"{n}\"")

    if not sections and not tables:
        print(f"\n  All named elements in document:")
        for r, n, _ in doc_elements:
            if n:
                print(f"    {r}: \"{n}\"")

    print(f"\n{'='*60}")
    print("RESULTS:")
    print(f"  'Descriptive Statistics' found: {desc_stats_found}")
    print(f"  'BoxPlots' found:            {boxplots_found}")

    if desc_stats_found and boxplots_found:
        print("  PASS - Both expected result elements accessible!")
        return 0
    else:
        print("  FAIL - Some expected elements missing")
        return 1


if __name__ == "__main__":
    sys.exit(main())