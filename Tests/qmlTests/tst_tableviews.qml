import QtQuick 2.15
import QtQuick.Test 2.15
import QMLComponents.JASP.Controls 1.0

TestCase {
    name: "tst_tableviews"
    when: windowShown

    TableView {
        id: tableView
    }

    SimpleTableView {
        id: simpleTableView
    }

    JagsTableView {
        id: jagsTableView
    }

    function test_tableviews_exist() {
        verify(tableView, "TableView should exist")
        verify(simpleTableView, "SimpleTableView should exist")
        verify(jagsTableView, "JagsTableView should exist")
    }
}
