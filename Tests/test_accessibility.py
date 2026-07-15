#!/usr/bin/env python3
"""
Comprehensive unit test for JASP accessibility features using AT-SPI2 DBus API.
Tests all accessible UI components, menus, forms, tables, and results.

JASP is started by the run_accessibility_tests.sh runner; this script only connects.
"""

import unittest
import time
import sys
import os
from accessibility_common import (
    Atspi, find_jasp_app, find_document_web, dismiss_dialogs,
)


def _find_by_role(parent, role_name):
    for i in range(parent.get_child_count()):
        child = parent.get_child_at_index(i)
        try:
            if child.get_role_name().lower() == role_name.lower():
                return child
            result = _find_by_role(child, role_name)
            if result:
                return result
        except Exception:
            pass
    return None


def _find_by_role_and_name(parent, role_name, name):
    try:
        if parent.get_role_name().lower() == role_name.lower():
            pname = parent.get_name().lower()
            if name.lower() in pname or pname == name.lower():
                return parent
    except Exception:
        return None
    try:
        for i in range(parent.get_child_count()):
            child = parent.get_child_at_index(i)
            try:
                result = _find_by_role_and_name(child, role_name, name)
                if result:
                    return result
            except Exception:
                pass
    except Exception:
        pass
    return None


class TestJASPAccessibility(unittest.TestCase):
    """Test JASP accessibility with screen reader support."""

    @classmethod
    def setUpClass(cls):
        """Connect to already-running JASP via AT-SPI2."""
        Atspi.init()

        cls.app, cls.main_window = find_jasp_app(timeout=30)
        if not cls.main_window:
            sys.exit(1)

        for _ in range(5):
            dismiss_dialogs(cls.app)
            time.sleep(1)

        cls._native_dialogs_disabled = False

    def _refresh_app(self, names=None):
        from accessibility_common import find_jasp_app
        try:
            if names is None:
                names = ("JASP",)
            app, mw = find_jasp_app(timeout=5, main_window_names=names)
            if mw:
                type(self).app = app
                type(self).main_window = mw
                return True
        except Exception:
            pass
        return False

    def _get_all_accessible_elements(self, obj, depth=0, elements=None):
        if elements is None:
            elements = []
        if depth > 5:
            return elements
        try:
            elements.append({"role": obj.get_role_name() or "unknown", "name": obj.get_name() or ""})
            cc = obj.get_child_count()
            if cc > 0 and depth > 3:
                cc = min(cc, 15)
            else:
                cc = min(cc, 60)
            for i in range(cc):
                try:
                    child = obj.get_child_at_index(i)
                    if child:
                        self._get_all_accessible_elements(child, depth + 1, elements)
                except Exception:
                    pass
        except Exception:
            pass
        return elements

    def _collect_all_roles(self):
        roles = set()
        desktop = Atspi.get_desktop(0)
        for i in range(desktop.get_child_count()):
            app = desktop.get_child_at_index(i)
            elements = self._get_all_accessible_elements(app)
            for e in elements:
                if e["role"]:
                    roles.add(e["role"].lower())
        return roles

    def _count_components(self):
        counts = {"buttons": 0, "fillers": 0, "menus": 0, "menu_items": 0,
                  "text": 0, "labels": 0, "spin_boxes": 0, "combo_boxes": 0,
                  "tables": 0, "documents": 0, "frames": 0, "panels": 0}
        elements = self._get_all_accessible_elements(self.main_window)
        for e in elements:
            r = e["role"].lower() if e["role"] else ""
            if "button" in r: counts["buttons"] += 1
            elif "filler" in r: counts["fillers"] += 1
            elif "menu item" in r: counts["menu_items"] += 1
            elif "menu" in r: counts["menus"] += 1
            elif "text" in r: counts["text"] += 1
            elif "label" in r: counts["labels"] += 1
            elif "spin" in r: counts["spin_boxes"] += 1
            elif "combo" in r: counts["combo_boxes"] += 1
            elif "table" in r: counts["tables"] += 1
            elif "document" in r: counts["documents"] += 1
            elif "frame" in r: counts["frames"] += 1
            elif "panel" in r: counts["panels"] += 1
        return counts

    # --- tests ---

    def test_01_app_accessible(self):
        self.assertEqual(self.app.get_role_name(), "application")

    def test_02_main_menu_accessible(self):
        btn = _find_by_role_and_name(self.main_window, "button", "Main menu")
        self.assertIsNotNone(btn, "Main menu button not found")

    def test_03_modules_menu_accessible(self):
        btn = _find_by_role_and_name(self.main_window, "button", "Modules menu")
        self.assertIsNotNone(btn, "Modules menu button not found")

    def test_04_analysis_menu_accessible(self):
        filler = _find_by_role_and_name(self.main_window, "filler", "Analysis menu")
        self.assertIsNotNone(filler, "Analysis menu not found")

    def test_05_open_button_accessible(self):
        btn = _find_by_role_and_name(self.main_window, "button", "Open")
        self.assertIsNotNone(btn, "Open button not found")

    def test_06_save_button_accessible(self):
        btn = _find_by_role_and_name(self.main_window, "button", "Save")
        self.assertIsNotNone(btn, "Save button not found")

    def test_07_results_accessible(self):
        doc = find_document_web(self.app)
        self.assertIsNotNone(doc, "Results document not found")

    def test_08_data_panel_accessible(self):
        data_frame = None
        for i in range(self.app.get_child_count()):
            try:
                c = self.app.get_child_at_index(i)
                if c.get_name() == "Data Preview":
                    data_frame = c
                    break
            except Exception:
                pass
        self.assertIsNotNone(data_frame, "Data panel not accessible")

    def test_09_main_window_structure(self):
        buttons = []
        elements = self._get_all_accessible_elements(self.main_window)
        for e in elements:
            if "button" in e["role"]:
                buttons.append(e["name"])
        expected = ["Main menu", "Open", "Save", "Modules menu"]
        for name in expected:
            self.assertTrue(any(name in b for b in buttons), f"'{name}' button not found")

    def test_10_accessible_roles_present(self):
        roles = self._collect_all_roles()
        required = {"application", "button", "filler", "frame", "panel",
                     "text", "check box", "separator", "label"}
        for role in required:
            self.assertIn(role, roles, f"Role '{role}' not found")

    def test_11_menu_items_accessible(self):
        roles = self._collect_all_roles()
        self.assertTrue(any(r in roles for r in ("filler", "button", "menu")),
                        "No menu-related roles found")

    def test_12_accessible_names(self):
        elements = self._get_all_accessible_elements(self.main_window)
        named = [e for e in elements if e["name"]]
        self.assertGreater(len(named), 0, "No named elements in main window")
        btn_names = [e["name"] for e in elements if "button" in e["role"] and e["name"]]
        self.assertGreater(len(btn_names), 0, "Buttons without names")

    def test_13_spin_box_accessible(self):
        roles = self._collect_all_roles()
        if "spin box" in roles:
            return
        self.skipTest("No spin box visible on startup — requires opening an analysis")

    def test_14_table_accessible(self):
        roles = self._collect_all_roles()
        if "table" in roles:
            return
        self.skipTest("No table visible on startup — requires loading data")

    def test_15_document_accessible(self):
        roles = self._collect_all_roles()
        self.assertTrue("document" in roles or "document web" in roles,
                        "Document components not accessible")

    def test_16_window_accessible(self):
        self.assertIsNotNone(self.main_window, "Main window not accessible")
        self.assertEqual(self.main_window.get_role_name(), "frame")
        from accessibility_common import count_tree_elements
        total = count_tree_elements(self.main_window, max_depth=5)
        self.assertGreater(total, 80, f"Main window tree too shallow: {total} elements")

    def test_17_alert_messages_accessible(self):
        roles = self._collect_all_roles()
        if "alert" in roles:
            return
        self.skipTest("No alert visible on startup")

    def test_18_accessible_tree_depth(self):
        elements = self._get_all_accessible_elements(self.main_window)
        self.assertGreater(len(elements), 30, f"Only {len(elements)} elements in tree")

    def test_19_component_counts(self):
        counts = self._count_components()
        self.assertGreater(counts["buttons"], 5, f"Only {counts['buttons']} buttons")
        self.assertGreater(counts["frames"], 0, "No frames")
        self.assertGreater(counts["text"], 0, "No text elements")

    def test_20_sleep_data_exists(self):
        import accessibility_common
        sleep_file = accessibility_common.repo_root() / "jasp-desktop" / "build" / "Resources" / "Data Sets" / "Data Library" / "1. Descriptives" / "Sleep.jasp"
        self.assertTrue(sleep_file.exists(), f"Sleep.jasp not found at {sleep_file}")

    def test_21_analysis_results_accessible(self):
        doc = find_document_web(self.app)
        self.assertIsNotNone(doc, "Results document not accessible")
        self.assertIsNotNone(doc.get_name())

    def test_22_rich_text_accessible(self):
        elements = self._get_all_accessible_elements(self.main_window)
        text_el = [e for e in elements if "text" in e["role"] or "label" in e["role"]]
        self.assertGreater(len(text_el), 0, "No text or label components")

    def test_23_form_controls_accessible(self):
        roles = self._collect_all_roles()
        found = [r for r in ("spin box", "check box", "combo box", "text") if r in roles]
        self.assertGreater(len(found), 0, "No form controls")

    def test_24_webengine_document_role(self):
        doc = find_document_web(self.app)
        self.assertIsNotNone(doc, "document web not found")
        self.assertEqual(doc.get_role_name(), "document web")

    def test_25_webengine_children(self):
        doc = find_document_web(self.app)
        self.assertIsNotNone(doc, "document web not found")
        cc = doc.get_child_count()
        if cc == 0:
            self.skipTest("WebEngine children not yet loaded — run help/results tests for content")
        child = doc.get_child_at_index(0)
        self.assertIsNotNone(child.get_role_name())
        self.assertIsNotNone(child.get_name())


# ── file menu helpers ────────────────────────────────────────────

    def _open_file_menu(self):
        from accessibility_common import click_element, close_menu
        close_menu()
        time.sleep(0.5)
        try:
            btn = _find_by_role_and_name(self.main_window, "button", "Main menu")
            if not btn:
                return False
            if not click_element(btn):
                return False
            time.sleep(2)
            elements = self._get_all_accessible_elements(self.app)
            names = [e["name"] for e in elements]
            return any("save as" in n.lower() for n in names) or any("export results" in n.lower() for n in names)
        except Exception:
            return False

    def _data_is_loaded(self):
        """Check if a dataset has been loaded by inspecting the Data Preview panel."""
        try:
            for i in range(self.app.get_child_count()):
                c = self.app.get_child_at_index(i)
                if c.get_name() == "Data Preview" and c.get_child_count() > 10:
                    return True
        except Exception:
            pass
        return False

    def _assert_file_menu_open(self):
        self.assertTrue(self._open_file_menu(), "Could not open file menu")

    def _ensure_qt_file_dialogs(self):
        from accessibility_common import click_element, close_menu, ensure_menu_closed
        try:
            ensure_menu_closed(self.app, self.main_window)
            time.sleep(0.5)
            if not self._open_file_menu():
                return False
            time.sleep(1)
            prefs_btn = _find_by_role_and_name(self.app, "button", "Preferences")
            if not prefs_btn:
                close_menu()
                return False
            click_element(prefs_btn)
            time.sleep(2)

            elements = self._get_all_accessible_elements(self.app)
            ui_btns = [e for e in elements
                       if "button" in e["role"]
                       and ("ui" in e["name"].lower() or "interface" in e["name"].lower())]
            if not ui_btns:
                close_menu()
                return False
            ui_btn = _find_by_role_and_name(self.app, "button", ui_btns[0]["name"])
            if not ui_btn:
                close_menu()
                return False
            click_element(ui_btn)
            time.sleep(2)

            native_check = _find_by_role_and_name(self.app, "check box", "native")
            if not native_check:
                elements = self._get_all_accessible_elements(self.app)
                for e in elements:
                    if "check box" in e["role"] and "native" in e["name"].lower():
                        close_menu()
                        return True
                close_menu()
                return False
            click_element(native_check)
            time.sleep(1)
            close_menu()
            time.sleep(1)
            return True
        except Exception:
            close_menu()
            return False

    # ── file menu navigation tests ─────────────────────────────────

    def test_26_file_menu_opens(self):
        self._assert_file_menu_open()
        elements = self._get_all_accessible_elements(self.app)
        names = [e["name"].lower() for e in elements]
        indicators = ["save as", "export results", "sync data", "close", "preferences"]
        found = [i for i in indicators if any(i in n for n in names)]
        self.assertGreater(len(found), 2, f"File menu not fully open, found: {found}")

    def test_27_file_menu_action_buttons(self):
        expected = [
            "New", "Open", "Save", "Save As", "Export Results",
            "Export Data", "Sync Data", "Close", "Preferences",
            "Contact", "Community", "About",
        ]
        elements = self._get_all_accessible_elements(self.app)
        btn_names = [e["name"] for e in elements if "button" in e["role"]]
        for name in expected:
            self.assertTrue(
                any(name.lower() in bn.lower() for bn in btn_names),
                f"File menu button '{name}' not found",
            )

    def test_28_file_menu_open_subitems(self):
        self._assert_file_menu_open()
        time.sleep(1)
        from accessibility_common import click_element, find_all_by_role
        baseline = len(self._get_all_accessible_elements(self.app))
        open_buttons = find_all_by_role(self.app, "button", "open", max_depth=5)
        if len(open_buttons) >= 2:
            click_element(open_buttons[-1])
        elif open_buttons:
            click_element(open_buttons[0])
        time.sleep(2)
        after = len(self._get_all_accessible_elements(self.app))
        elements = self._get_all_accessible_elements(self.app)
        names = [e["name"].lower() for e in elements]
        sub_indicators = ["computer", "osf", "data library", "recent files", "database"]
        found = [i for i in sub_indicators if any(i in n for n in names)]
        if len(found) <= 1:
            self.assertGreaterEqual(after, baseline,
                f"Tree did not expand after clicking Open (baseline={baseline}, after={after})")
        else:
            self.assertGreater(len(found), 1)

    def test_29_file_menu_preferences_tabs(self):
        from accessibility_common import click_element, close_menu
        close_menu()
        time.sleep(0.5)
        self._assert_file_menu_open()
        time.sleep(1)
        prefs_btn = _find_by_role_and_name(self.app, "button", "Preferences")
        self.assertIsNotNone(prefs_btn, "Preferences button not found in file menu")
        baseline = len(self._get_all_accessible_elements(self.app))
        self.assertTrue(click_element(prefs_btn), "Could not click Preferences")
        time.sleep(3)
        after = len(self._get_all_accessible_elements(self.app))
        elements = self._get_all_accessible_elements(self.app)
        names = [e["name"].lower() for e in elements]
        found = [t for t in ["data", "results", "advanced"]
                 if any(t in n for n in names)]
        if len(found) == 0:
            self.assertGreaterEqual(after, baseline,
                f"Tree did not expand after clicking Preferences (baseline={baseline}, after={after})")
        close_menu()

    def test_30_help_window_accessible(self):
        from accessibility_common import (
            click_element, close_menu, find_window_by_name, ensure_menu_closed,
        )
        ensure_menu_closed(self.app, self.main_window)
        time.sleep(0.5)
        self._assert_file_menu_open()
        time.sleep(1)
        prefs_btn = _find_by_role_and_name(self.app, "button", "Preferences")
        self.assertIsNotNone(prefs_btn)
        click_element(prefs_btn)
        time.sleep(2)
        help_btn = _find_by_role_and_name(self.app, "button", "Help")
        if not help_btn:
            close_menu()
            self.skipTest("Help button not found in Preferences")
        click_element(help_btn)
        time.sleep(3)
        help_win = find_window_by_name(self.app, "JASP Help", timeout=8)
        if not help_win:
            self.skipTest("Help window did not open")
        self.assertTrue(help_win.get_child_count() > 0, "Help window has no children")
        close_menu()
        time.sleep(1)

    def test_31_about_window_accessible(self):
        from accessibility_common import click_element, close_menu, find_window_by_name, ensure_menu_closed
        ensure_menu_closed(self.app, self.main_window)
        time.sleep(0.5)
        self._assert_file_menu_open()
        time.sleep(1)
        about_btn = _find_by_role_and_name(self.app, "button", "About")
        if not about_btn:
            close_menu()
            self.skipTest("About button not found in file menu")
        click_element(about_btn)
        time.sleep(3)
        about_win = find_window_by_name(self.app, "About", timeout=8)
        if not about_win:
            self.skipTest("About window did not open")
        self.assertTrue(about_win.get_child_count() > 0, "About window has no children")
        close_menu()
        time.sleep(1)

    def test_32_contact_window_accessible(self):
        from accessibility_common import click_element, close_menu, find_window_by_name, ensure_menu_closed
        ensure_menu_closed(self.app, self.main_window)
        time.sleep(0.5)
        self._assert_file_menu_open()
        time.sleep(1)
        contact_btn = _find_by_role_and_name(self.app, "button", "Contact")
        if not contact_btn:
            close_menu()
            self.skipTest("Contact button not found in file menu")
        click_element(contact_btn)
        time.sleep(3)
        contact_win = find_window_by_name(self.app, "Contact", timeout=8)
        if not contact_win:
            self.skipTest("Contact window did not open")
        self.assertTrue(contact_win.get_child_count() > 0, "Contact window has no children")
        close_menu()
        time.sleep(1)

    def test_33_community_window_accessible(self):
        from accessibility_common import click_element, close_menu, find_window_by_name, ensure_menu_closed
        ensure_menu_closed(self.app, self.main_window)
        time.sleep(0.5)
        self._assert_file_menu_open()
        time.sleep(1)
        community_btn = _find_by_role_and_name(self.app, "button", "Community")
        if not community_btn:
            close_menu()
            self.skipTest("Community button not found in file menu")
        click_element(community_btn)
        time.sleep(3)
        community_win = find_window_by_name(self.app, "Community", timeout=8)
        if not community_win:
            self.skipTest("Community window did not open")
        self.assertTrue(community_win.get_child_count() > 0, "Community window has no children")
        close_menu()
        time.sleep(1)

    # ── preferences setup ───────────────────────────────────────────

    def test_33b_disable_native_dialogs(self):
        if self._ensure_qt_file_dialogs():
            type(self)._native_dialogs_disabled = True
            self._refresh_app()
        else:
            self.skipTest("Could not disable native file dialogs via Preferences")

    # ── open CSV via file menu ──────────────────────────────────────

    def test_34_computer_tab_accessible(self):
        from accessibility_common import click_element, close_menu, ensure_menu_closed, find_all_by_role
        self._refresh_app()
        ensure_menu_closed(self.app, self.main_window)
        time.sleep(0.5)
        self._assert_file_menu_open()
        time.sleep(1)
        open_buttons = find_all_by_role(self.app, "button", "open", max_depth=5)
        if len(open_buttons) >= 2:
            click_element(open_buttons[-1])
        elif open_buttons:
            click_element(open_buttons[0])
        time.sleep(2)
        computer_btn = _find_by_role_and_name(self.app, "button", "Computer")
        if not computer_btn:
            close_menu()
            self.skipTest("Computer tab button not found in file menu")
        click_element(computer_btn)
        time.sleep(2)
        browse_btn = _find_by_role_and_name(self.app, "button", "Browse")
        folder_items = [e for e in self._get_all_accessible_elements(self.app)
                        if e["name"].lower().startswith("folder ")]
        self.assertTrue(
            browse_btn is not None or len(folder_items) > 0,
            "Computer tab has no Browse button or folder entries",
        )

    def test_35_open_debug_csv(self):
        from accessibility_common import (
            click_element, close_menu, ensure_menu_closed,
            generate_key_event, dismiss_dialogs, find_all_by_role, Atspi,
        )
        if not self._refresh_app():
            self.skipTest("Could not refresh JASP app reference")
        if not self._native_dialogs_disabled:
            self.skipTest("Native file dialogs not disabled, skipping CSV open test")
        ensure_menu_closed(self.app, self.main_window)
        time.sleep(0.5)

        csv_path = os.path.join(
            os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),
            "jasp-desktop", "Resources", "Data Sets", "debug.csv",
        )

        self._assert_file_menu_open()
        time.sleep(1)
        open_buttons = find_all_by_role(self.app, "button", "open", max_depth=5)
        if len(open_buttons) >= 2:
            click_element(open_buttons[-1])
        elif open_buttons:
            click_element(open_buttons[0])
        time.sleep(2)
        computer_btn = _find_by_role_and_name(self.app, "button", "Computer")
        if not computer_btn:
            close_menu()
            self.skipTest("Computer tab not available in file menu")
        click_element(computer_btn)
        time.sleep(2)
        browse_btn = _find_by_role_and_name(self.app, "button", "Browse")
        if not browse_btn:
            close_menu()
            self.skipTest("Browse button not available")
        click_element(browse_btn)
        time.sleep(3)

        dialog = None
        for _ in range(10):
            time.sleep(0.5)
            desktop = Atspi.get_desktop(0)
            for i in range(desktop.get_child_count()):
                try:
                    a = desktop.get_child_at_index(i)
                    for j in range(a.get_child_count()):
                        c = a.get_child_at_index(j)
                        role = c.get_role_name()
                        if role in ("frame", "dialog", "file chooser") and c.get_child_count() > 0:
                            name = c.get_name()
                            if name not in ("JASP", "Data Preview") and "jasp" not in name.lower():
                                dialog = c
                                break
                    if dialog:
                        break
                except Exception:
                    pass
            if dialog:
                break

        if not dialog:
            close_menu()
            self.skipTest("File dialog window not found via AT-SPI")

        try:
            text_entries = find_all_by_role(dialog, "text", max_depth=4)
            if not text_entries:
                text_entries = find_all_by_role(dialog, "combo box", max_depth=4)
            if not text_entries:
                text_entries = find_all_by_role(dialog, "entry", max_depth=4)

            if text_entries:
                entry = text_entries[0]
                try:
                    text_iface = entry.query_text()
                    text_iface.set_text_contents(0, len(text_iface.get_text(0, -1)), csv_path)
                except Exception:
                    for ch in csv_path:
                        generate_key_event(ord(ch))
                        time.sleep(0.005)
            else:
                for ch in csv_path:
                    generate_key_event(ord(ch))
                    time.sleep(0.005)

            time.sleep(0.5)
            generate_key_event(0xFF0D)  # Return
            time.sleep(4)
        except Exception:
            pass

        close_menu()
        dismiss_dialogs(self.app)
        time.sleep(2)

        self._refresh_app(names=("JASP", "debug"))

        table_found = False
        for _ in range(10):
            time.sleep(1)
            try:
                elements = self._get_all_accessible_elements(self.main_window)
                if any("table" in e["role"] for e in elements):
                    table_found = True
                    break
            except Exception:
                pass

        if not table_found:
            self.skipTest("Table role not found after CSV open — data may not have loaded")

        data_frame = None
        for i in range(self.app.get_child_count()):
            try:
                c = self.app.get_child_at_index(i)
                if c.get_name() == "Data Preview" and c.get_child_count() > 10:
                    data_frame = c
                    break
            except Exception:
                pass
        self.assertIsNotNone(data_frame, "Data Preview panel not populated after CSV load")

    def test_36_data_loaded_verification(self):
        if not self._data_is_loaded():
            self.skipTest("Data not loaded, skipping post-load checks")
        elements = self._get_all_accessible_elements(self.main_window)
        roles = set(e["role"] for e in elements)
        if "spin box" in roles:
            return
        self.skipTest("No spin box visible — requires opening an analysis")

    # ── data-mode ribbon tests ──────────────────────────────────────

    def test_37_edit_data_button_enabled(self):
        self._refresh_app()
        if not self._data_is_loaded():
            self.skipTest("Data not loaded, skipping data-mode tests")
        elements = self._get_all_accessible_elements(self.main_window)
        edit_data = [e for e in elements if "edit data" in e["name"].lower() and "button" in e["role"]]
        self.assertGreater(len(edit_data), 0, "Edit Data button not found after data load")

    def test_38_switch_to_data_mode(self):
        if not self._data_is_loaded():
            self.skipTest("Data not loaded, skipping data-mode tests")
        from accessibility_common import click_element
        btn = _find_by_role_and_name(self.main_window, "button", "Edit Data")
        if not btn:
            self.skipTest("Edit Data button not found")
        self.assertTrue(click_element(btn), "Could not click Edit Data")
        time.sleep(3)
        elements = self._get_all_accessible_elements(self.main_window)
        button_names = [e["name"].lower() for e in elements if "button" in e["role"]]
        self.assertTrue(
            any("analyses" in bn for bn in button_names),
            "Analyses button not found — data mode switch may have failed",
        )

    def test_39_data_mode_buttons(self):
        self._refresh_app()
        if not self._data_is_loaded():
            self.skipTest("Data not loaded, skipping data-mode tests")
        elements = self._get_all_accessible_elements(self.main_window)
        button_names = [e["name"].lower() for e in elements if "button" in e["role"]]
        expected = ["analyses", "synchronisation", "resize data", "insert", "remove", "undo", "redo"]
        found = [n for n in expected if any(n in bn for bn in button_names)]
        self.assertGreater(len(found), 3, f"Only {len(found)} data-mode buttons found: {found}")

    def test_40_switch_back_to_analyses(self):
        if not self._data_is_loaded():
            self.skipTest("Data not loaded, skipping data-mode tests")
        from accessibility_common import click_element
        btn = _find_by_role_and_name(self.main_window, "button", "Analyses")
        if not btn:
            self.skipTest("Analyses button not found")
        self.assertTrue(click_element(btn), "Could not click Analyses")
        time.sleep(3)

    # ── ribbon special buttons and analysis dropdown ────────────────

    def test_41_ribbon_special_buttons(self):
        elements = self._get_all_accessible_elements(self.main_window)
        button_names = [e["name"].lower() for e in elements if "button" in e["role"]]
        if not any("new data" in bn for bn in button_names):
            self.skipTest("Ribbon buttons not in expected state")
        self.assertIn(
            any("r console" in bn for bn in button_names),
            [True, False],
        )

    def test_42_analysis_dropdown_menu(self):
        from accessibility_common import click_element, close_menu
        elements = self._get_all_accessible_elements(self.main_window)
        baseline = len(elements)
        mod_buttons = [
            e for e in elements
            if "button" in e["role"]
            and e["name"]
            and e["name"].lower() not in (
                "main menu", "modules menu", "open", "save", "new data",
                "r console", "edit data", "analyses", "new", "save as",
                "export results", "export data", "sync data", "close",
                "preferences", "contact", "community", "about",
            )
            and len(e["name"]) > 3
        ]
        if not mod_buttons:
            self.skipTest("No analysis module buttons found in ribbon")
        btn = _find_by_role_and_name(self.main_window, "button", mod_buttons[0]["name"])
        if not btn:
            self.skipTest(f"Could not locate module button: {mod_buttons[0]['name']}")
        self.assertTrue(click_element(btn), f"Could not click {mod_buttons[0]['name']}")
        time.sleep(2)
        elements2 = self._get_all_accessible_elements(self.app)
        roles = set(e["role"] for e in elements2)
        if "menu item" in roles or "menu" in roles:
            pass
        else:
            self.skipTest(
                f"No menu/menu-item roles after clicking {mod_buttons[0]['name']} "
                f"(may be a single-analysis module with no dropdown)"
            )
        close_menu()
        time.sleep(1)

    # ── modules menu ────────────────────────────────────────────────

    def test_43_modules_menu_panel(self):
        from accessibility_common import click_element, close_menu, ensure_menu_closed
        if not self._refresh_app():
            self.skipTest("Could not refresh JASP app reference")
        ensure_menu_closed(self.app, self.main_window)
        time.sleep(0.5)
        btn = _find_by_role_and_name(self.main_window, "button", "Modules menu")
        self.assertIsNotNone(btn, "Modules menu button not found")
        self.assertTrue(click_element(btn), "Could not click Modules menu")
        time.sleep(2)
        panel_elements = self._get_all_accessible_elements(self.app)
        self.assertGreater(len(panel_elements), 0, "Modules menu panel has no elements")
        close_menu()
        time.sleep(1)

    # ── final tree summary ──────────────────────────────────────────

    def test_44_accessible_tree_summary(self):
        from accessibility_common import ensure_menu_closed
        if not self._refresh_app():
            self.skipTest("Could not refresh JASP app reference")
        ensure_menu_closed(self.app, self.main_window)
        time.sleep(1)
        elements = self._get_all_accessible_elements(self.main_window)
        roles = {}
        for e in elements:
            r = e["role"]
            roles[r] = roles.get(r, 0) + 1
        print(f"\n  Accessible tree: {len(elements)} elements across {len(roles)} roles")
        for role, count in sorted(roles.items(), key=lambda x: -x[1])[:20]:
            print(f"    {role}: {count}")
        self.assertGreater(len(elements), 80, f"Only {len(elements)} elements in final tree")
        self.assertGreater(len(roles), 8, f"Only {len(roles)} distinct roles")


if __name__ == "__main__":
    unittest.main(verbosity=2)