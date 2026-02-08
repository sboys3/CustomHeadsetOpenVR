import { Component, OnInit, OnDestroy, ChangeDetectionStrategy, inject } from '@angular/core';
import { SystemReadyComponent } from '../../utilities/system-ready/system-ready.component';
import { MeganexX8KComponent } from '../devices/meganex-x8-k/meganex-x8-k.component';
import { MatTabsModule } from '@angular/material/tabs';
import { GeneralComponent } from '../devices/general/general.component';
import { HeadsetType, HeadsetType as HeadsetTypeEnum } from '../../services/JsonFileDefines';
import { DriverInfoService } from '../../services/driver-info.service';
import { CommonModule } from '@angular/common';
import { Subscription, Subject } from 'rxjs';
import { effect, signal, computed } from '@angular/core';
import {MatIconModule} from '@angular/material/icon'
import {SystemDiagnosticService} from '../../services/system-diagnostic.service'
import {MatButtonModule} from '@angular/material/button'

export interface TabConfig {
  type: string;
  headsetType: HeadsetType;
}

@Component({
    selector: 'app-driver-settings',
    imports: [
        SystemReadyComponent,
        MeganexX8KComponent,
        GeneralComponent,
        MatTabsModule,
        MatIconModule,
        MatButtonModule,
        CommonModule
    ],
    providers: [MeganexX8KComponent],
    templateUrl: './driver-settings.component.html',
    styleUrl: './driver-settings.component.scss',
    changeDetection: ChangeDetectionStrategy.OnPush
})
export class DriverSettingsComponent implements OnInit, OnDestroy {
    // Tab configurations
    private _tabs = signal<TabConfig[]>([]);
    public currentHeadsetType = signal<HeadsetType | undefined>(undefined);
    private _selectedTab = signal<TabConfig | undefined>(undefined);
    private _previousOrderedTabs: TabConfig[] = [];
    public sds = inject(SystemDiagnosticService)
    driverEnablePrompt = signal(false)
    nonNativeWarning = signal(false)

    // Expose component classes to template
    MeganexX8KComponent = MeganexX8KComponent;
    GeneralComponent = GeneralComponent;

    constructor(private dis: DriverInfoService) {
        // Register tab configurations
        this._tabs.set([
            { type: 'General', headsetType: HeadsetType.Other }, // Other
            { type: 'MeganeX8K', headsetType: HeadsetType.MeganeX8K }, // MeganeX8K
        ]);

        // Effect to update current headset type when driver info changes
        effect(() => {
            const info = this.dis.values();
            if (info && info.connectedHeadset !== undefined && info.connectedHeadset !== HeadsetType.None) {
                // Convert number to HeadsetType enum
                const headsetType = info.connectedHeadset as HeadsetType;
                this.currentHeadsetType.set(headsetType);
            }
            if (info && info.nonNativeHeadsetFound) {
                this.nonNativeWarning.set(true)
            } else {
                this.nonNativeWarning.set(false)
            }
        });

        // Effect to handle tab reordering and selection when headset changes
        effect(() => {
            const ordered = this.orderedTabs;
            const currentType = this.currentHeadsetType();
            const selected = this._selectedTab();
            

            // Check if order changed
            const orderChanged = !this.arraysEqual(ordered, this._previousOrderedTabs);
            
            setTimeout(() => {
                if (orderChanged) {
                    // If order changed, select the first tab
                    if (ordered.length > 0) {
                        this._selectedTab.set(ordered[0]);
                    }
                } else if (!selected && ordered.length > 0) {
                    // If no selected tab yet, select the first one
                    this._selectedTab.set(ordered[0]);
                }
            }, 0);
            
            
            console.log('Tab reordering triggered:', {
                ordered,
                currentType,
                selected,
                newSelected: this._selectedTab(),
                previous: this._previousOrderedTabs
            });
            

            // Update previous ordered tabs
            this._previousOrderedTabs = [...ordered];
        });
        
        effect(() => {
            const steamVrConfig = this.sds.steamVrConfig();
            if (steamVrConfig) {
                let customEnabled = this.sds.getSteamVRDriverEnableState(steamVrConfig, 'CustomHeadsetOpenVR')
                this.driverEnablePrompt.set(!customEnabled);
            }
        })
    }

    ngOnInit(): void {
    }

    ngOnDestroy(): void {
    }

    // Expose ordered tabs to template (getter that returns array)
    get orderedTabs(): TabConfig[] {
        const currentType = this.currentHeadsetType();
        const tabs = this._tabs();
        const previousOrder = this._previousOrderedTabs.length > 0 ? this._previousOrderedTabs : tabs;

        if (!currentType) {
            // When headset is unknown, keep tab order
            return previousOrder;
        }

        // Find the tab that matches the current headset type
        const specificTab = tabs.find(tab => tab.headsetType === currentType);

        if (!specificTab) {
            // If we can't find the tab for the headset type, use the default order
            return tabs;
        }

        // Reorder: specific headset tab first, then general tab
        const otherTabs = tabs.filter(tab => tab !== specificTab);
        return [specificTab, ...otherTabs];
    }

    // Get the selected tab index
    get selectedIndex(): number {
        const selected = this._selectedTab();
        const ordered = this.orderedTabs;
        if (selected) {
            return ordered.indexOf(selected);
        }
        return 0;
    }

    // Handle tab change
    onTabChange(index: number): void {
        const ordered = this.orderedTabs;
        if (ordered[index]) {
            this._selectedTab.set(ordered[index]);
        }
    }
    
    // Compare two arrays for equality based on object references
    private arraysEqual(a: TabConfig[], b: TabConfig[]): boolean {
        if (a.length !== b.length) return false;
        for (let i = 0; i < a.length; i++) {
            if (a[i] !== b[i]) return false;
        }
        return true;
    }
    
    async enableDriver() {
        await this.sds.enableSteamVRDriver("CustomHeadsetOpenVR")
    }
}
