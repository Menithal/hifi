//
//  Created by Matti 'Menithal' Lahtinen on 29/10/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.0
import QtQuick.Controls 1.4 as Original
import QtQuick.Controls.Styles 1.4
import "../../styles-uit"
import "../../controls"
import "../../controls-uit" as HifiControls
import "."


Rectangle { 
    id: genericConfiguration

    width: parent.width
    height: parent.height
    anchors.fill: parent

  Row {
        HifiControls.CheckBox {
            id: "enabled"
            width: 15
            height: 15
            boxRadius: 7

            onClicked: {

            }
        }
        RalewayBold {
            size: 12
            text: "Enabled"
            color: hifi.colors.lightGrayText
        }
        
    }
}