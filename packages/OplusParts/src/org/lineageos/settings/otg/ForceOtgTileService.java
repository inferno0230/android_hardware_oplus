/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package org.lineageos.settings.otg;

import android.service.quicksettings.Tile;
import android.service.quicksettings.TileService;

import org.lineageos.settings.R;

public class ForceOtgTileService extends TileService {

    @Override
    public void onStartListening() {
        super.onStartListening();
        refreshTileState();
    }

    @Override
    public void onClick() {
        super.onClick();

        if (!ForceOtgUtils.isSupported()) {
            refreshTileState();
            return;
        }

        final boolean isEnabled = ForceOtgUtils.isForceOtgEnabled(this);
        ForceOtgUtils.setForceOtgEnabled(this, !isEnabled);
        refreshTileState();
    }

    private void refreshTileState() {
        final Tile tile = getQsTile();
        if (tile == null) {
            return;
        }

        final boolean isSupported = ForceOtgUtils.isSupported();
        final boolean isEnabled = isSupported && ForceOtgUtils.isForceOtgEnabled(this);

        tile.setLabel(getString(R.string.force_otg_tile_title));
        if (!isSupported) {
            tile.setState(Tile.STATE_UNAVAILABLE);
        } else {
            tile.setState(isEnabled ? Tile.STATE_ACTIVE : Tile.STATE_INACTIVE);
        }
        tile.updateTile();
    }
}
