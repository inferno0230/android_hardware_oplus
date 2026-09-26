/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package org.lineageos.settings.ltpo;

import android.service.quicksettings.Tile;
import android.service.quicksettings.TileService;

import org.lineageos.settings.R;

public class LtpoTileService extends TileService {

    @Override
    public void onStartListening() {
        super.onStartListening();
        refreshTileState();
    }

    @Override
    public void onClick() {
        super.onClick();

        if (!LtpoHelper.isSupported()) {
            refreshTileState();
            return;
        }

        final boolean isEnabled = LtpoHelper.isLtpoEnabled(this);
        LtpoHelper.setLtpoEnabled(this, !isEnabled);
        refreshTileState();
    }

    private void refreshTileState() {
        final Tile tile = getQsTile();
        if (tile == null) {
            return;
        }

        final boolean isSupported = LtpoHelper.isSupported();
        final boolean isEnabled = isSupported && LtpoHelper.isLtpoEnabled(this);

        tile.setLabel(getString(R.string.ltpo_tile_title));
        if (!isSupported) {
            tile.setState(Tile.STATE_UNAVAILABLE);
        } else {
            tile.setState(isEnabled ? Tile.STATE_ACTIVE : Tile.STATE_INACTIVE);
        }
        tile.updateTile();
    }
}
