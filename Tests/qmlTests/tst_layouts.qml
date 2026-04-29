import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_layouts"
    when: windowShown

    ColumnLayout {
        id: colLayout
    }

    RowLayout {
        id: rowLayout
    }

    GridLayout {
        id: gridLayout
    }

    function test_layouts_exist() {
        verify(colLayout, "ColumnLayout should exist")
        verify(rowLayout, "RowLayout should exist")
        verify(gridLayout, "GridLayout should exist")
    }
}
