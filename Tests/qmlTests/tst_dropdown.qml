import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_dropdown"
    when: windowShown

    DropDown {
        id: dropdown
        model: ["A", "B", "C"]
    }

    function test_dropdown_model_size() {
        compare(dropdown.count, 3, "Dropdown should have 3 items")
    }

    function test_dropdown_selection() {
        dropdown.currentIndex = 1
        compare(dropdown.currentText, "B", "Dropdown text should update on selection")
    }
}
