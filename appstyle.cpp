#include "appstyle.h"

QString moneySharkStyleSheet()
{
    return QStringLiteral(R"(
        QWidget {
            background: #071421;
            color: #edf6ff;
            font-family: "Segoe UI", Arial, sans-serif;
            font-size: 14px;
        }
        QMainWindow, QDialog {
            background: #071421;
        }
        QMenuBar {
            background: #0b1f33;
            color: #edf6ff;
            border-bottom: 1px solid #173858;
        }
        QMenuBar::item {
            padding: 7px 12px;
            background: transparent;
        }
        QMenuBar::item:selected, QMenu::item:selected {
            background: #155a94;
        }
        QMenu {
            background: #0d253c;
            color: #edf6ff;
            border: 1px solid #1e4a72;
        }
        QStatusBar {
            background: #0b1f33;
            color: #a9bfd4;
        }
        QLabel#pageTitle {
            font-size: 24px;
            font-weight: 700;
            padding: 14px;
            border-radius: 8px;
            background: #0d253c;
            border: 1px solid #1e4a72;
        }
        QLabel#balanceLabel {
            font-size: 18px;
            font-weight: 700;
            padding: 10px 14px;
            border-radius: 8px;
            background: #102d49;
            border: 1px solid #2a6da8;
        }
        QWidget#panel, QGroupBox {
            background: #0d253c;
            border: 1px solid #1e4a72;
            border-radius: 8px;
        }
        QGroupBox {
            margin-top: 16px;
            padding: 16px 10px 10px 10px;
            font-weight: 700;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 5px;
        }
        QPushButton {
            background: #155a94;
            border: 1px solid #2b7fc2;
            color: #f5fbff;
            border-radius: 8px;
            padding: 10px 14px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #1b6dae;
        }
        QPushButton:pressed {
            background: #0f4774;
        }
        QPushButton:disabled {
            background: #233649;
            color: #7f91a4;
            border-color: #34495d;
        }
        QPushButton[secondary="true"] {
            background: #17324d;
            border-color: #315d86;
        }
        QPushButton[danger="true"] {
            background: #8b2d3b;
            border-color: #bd4053;
        }
        QPushButton[success="true"] {
            background: #19744d;
            border-color: #25a16e;
        }
        QLineEdit, QTextEdit, QComboBox, QSpinBox {
            background: #081a2b;
            color: #edf6ff;
            border: 1px solid #28597f;
            border-radius: 6px;
            padding: 8px;
            selection-background-color: #2b7fc2;
        }
        QLineEdit:focus, QTextEdit:focus, QComboBox:focus {
            border: 1px solid #4ca3ea;
        }
        QComboBox::drop-down {
            border: 0;
            width: 26px;
        }
        QComboBox QAbstractItemView {
            background: #0d253c;
            color: #edf6ff;
            selection-background-color: #155a94;
            border: 1px solid #28597f;
        }
        QTableWidget, QTableView {
            background: #081a2b;
            alternate-background-color: #0d253c;
            color: #edf6ff;
            gridline-color: #244b6f;
            border: 1px solid #1e4a72;
            border-radius: 6px;
        }
        QHeaderView::section {
            background: #102d49;
            color: #edf6ff;
            border: 0;
            border-right: 1px solid #244b6f;
            padding: 8px;
            font-weight: 700;
        }
        QTableWidget::item:selected, QTableView::item:selected {
            background: #155a94;
            color: #ffffff;
        }
        QTabWidget::pane {
            border: 1px solid #1e4a72;
            border-radius: 8px;
            background: #0d253c;
        }
        QTabBar::tab {
            background: #102d49;
            color: #cde6ff;
            padding: 9px 16px;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background: #155a94;
            color: #ffffff;
        }
        QMessageBox {
            background: #071421;
        }
        QMessageBox QLabel {
            color: #edf6ff;
        }
    )");
}
