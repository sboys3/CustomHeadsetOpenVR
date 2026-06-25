import { Component, OnInit, OnDestroy, ChangeDetectionStrategy, inject } from '@angular/core';
import { SystemReadyComponent } from '../../utilities/system-ready/system-ready.component';
import { MeganexX8KComponent } from '../devices/meganex-x8-k/meganex-x8-k.component';
import { DreamAirComponent } from '../devices/dream-air/dream-air.component';
import { DreamAirSeComponent } from '../devices/dream-air-se/dream-air-se.component';
import { CrystalSuperMicroOledComponent } from '../devices/crystal-super-micro-oled/crystal-super-micro-oled.component';
import { Pimax8KxComponent } from '../devices/pimax-8kx/pimax-8kx.component';
import { MatTabsModule } from '@angular/material/tabs';
import { GeneralComponent } from '../devices/general/general.component';
import { HeadsetType, HeadsetType as HeadsetTypeEnum, Settings } from '../../services/JsonFileDefines';
import { DriverInfoService } from '../../services/driver-info.service';
import { DriverSettingService } from '../../services/driver-setting.service';
import { CommonModule } from '@angular/common';
import { Subscription, Subject } from 'rxjs';
import { effect, signal, computed } from '@angular/core';
import {MatIconModule} from '@angular/material/icon'
import {SystemDiagnosticService} from '../../services/system-diagnostic.service'
import {MatButtonModule} from '@angular/material/button'
import { AppSettingService } from '../../services/app-setting.service';
import { AppUpdateService } from '../../services/app-update.service'
import { vendor, vendorUi, customHeadsetDriverName } from '../../../environment'

export interface TabConfig {
  type: string;
  headsetType: HeadsetType;
}

@Component({
    selector: 'app-driver-settings',
    imports: [
        SystemReadyComponent,
        MeganexX8KComponent,
        DreamAirComponent,
        DreamAirSeComponent,
        CrystalSuperMicroOledComponent,
        Pimax8KxComponent,
        GeneralComponent,
        MatTabsModule,
        MatIconModule,
        MatButtonModule,
        CommonModule
    ],
    providers: [MeganexX8KComponent, DreamAirComponent, DreamAirSeComponent, CrystalSuperMicroOledComponent, Pimax8KxComponent],
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
    driverBlocked = signal(false)
    // For vendor-specific drivers: tracks if the neutral driver is enabled (causing lockout)
    neutralDriverEnabled = signal(false)
    nonNativeWarning = signal(false)
    webView2Outdated = signal(false)
    webView2Version = signal<string | null>(null)

    private checkWebView2Version() {
        // versions I have seen that do not work: 100, 122
        const ua = navigator.userAgent;
        const edgeMatch = ua.match(/Edg\/([\d.]+)/);
        if (!edgeMatch) {
            this.webView2Outdated.set(false);
            this.webView2Version.set(null);
            return;
        }
        const versionString = edgeMatch[1];
        this.webView2Version.set(versionString);
        const versionParts = versionString.split('.').map(Number);
        // const minimumVersion = 140;
        const minimumVersion = 135;
        let isOutdated = versionParts[0] < minimumVersion;
        this.webView2Outdated.set(isOutdated);
    }

    // Expose component classes and enums to template
    MeganexX8KComponent = MeganexX8KComponent;
    GeneralComponent = GeneralComponent;
    DreamAirComponent = DreamAirComponent;
    DreamAirSeComponent = DreamAirSeComponent;
    CrystalSuperMicroOledComponent = CrystalSuperMicroOledComponent;
    Pimax8KxComponent = Pimax8KxComponent;
    HeadsetType = HeadsetTypeEnum;

    constructor(private dis: DriverInfoService, private dss: DriverSettingService, private appSettingService: AppSettingService, public appUpdateService: AppUpdateService) {
        // Register tab configurations
        let availableTabs = [];
        availableTabs.push({ type: 'General', headsetType: HeadsetType.Other }) // other headsets
        
        
        let onSettingsAvailable = (callback: (settings: Settings) => void) =>{
            let effectRef = effect(() => {
                let settings = this.dss.values()
                if(settings){
                    // no clue why the setTimeout is necessary, but everything breaks without it
                    setTimeout(() => {
                        settings = this.dss.values()
                        if(settings){
                            callback(settings)
                            effectRef.destroy()
                        }
                    }, 1000)
                }
            })
        }
        
        if(!vendorUi || vendorUi == "shiftall"){
            availableTabs.push({ type: 'MeganeX8K', headsetType: HeadsetType.MeganeX8K })
        }else{
            // disable when not shown
            onSettingsAvailable((settings) => {
                settings.meganeX8K.enable = false
                this.dss.save(settings)
            })
        }
        
        if(!vendorUi || vendorUi == "pimax"){
            availableTabs.push({ type: 'DreamAir', headsetType: HeadsetType.DreamAir })
            availableTabs.push({ type: 'DreamAirSE', headsetType: HeadsetType.DreamAirSE })
            availableTabs.push({ type: 'CrystalSuperMicroOLED', headsetType: HeadsetType.CrystalSuperMicroOLED })
            availableTabs.push({ type: 'Pimax8KX', headsetType: HeadsetType.Pimax8KX })
        }else{
            // disable when not shown
            onSettingsAvailable((settings) => {
                settings.dreamAir.enable = false
                settings.dreamAirSE.enable = false
                settings.crystalSuperMicroOLED.enable = false
                settings.pimax8KX.enable = false
                this.dss.save(settings)
            })
        }
        
        this._tabs.set(availableTabs)
        
        
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
                let customEnabled = this.sds.getSteamVRDriverEnableState(steamVrConfig, customHeadsetDriverName)
                this.driverEnablePrompt.set(!customEnabled);
                this.driverBlocked.set(this.sds.isDriverBlocked(steamVrConfig, customHeadsetDriverName));
                
                // For vendor-specific drivers, also check if the neutral driver is enabled
                // If so, show the enable prompt and track the lockout state
                if (vendor) {
                    const neutralEnabled = this.sds.getNeutralDriverEnabled(steamVrConfig);
                    this.neutralDriverEnabled.set(neutralEnabled);
                    // Show prompt if vendor driver is disabled OR if neutral driver is enabled (lockout)
                    this.driverEnablePrompt.set(!customEnabled || neutralEnabled);
                }
            }
        })
    }

    ngOnInit(): void {
        this.checkWebView2Version();
    }

    ngOnDestroy(): void {
    }

    // Expose ordered tabs to template (getter that returns array)
    get orderedTabs(): TabConfig[] {
        const currentType = this.currentHeadsetType();
        const tabs = this._tabs();
        const previousOrder = this._previousOrderedTabs.length > 0 ? this._previousOrderedTabs : tabs;
        const appSettings = this.appSettingService.values();
        const defaultTab = appSettings?.defaultSettingsTab ?? 'auto';

        // If default tab is set to a specific tab (not 'auto'), use that tab
        if (defaultTab !== 'auto') {
            const selectedTab = tabs.find(tab => tab.type === defaultTab);
            if (selectedTab) {
                const otherTabs = tabs.filter(tab => tab !== selectedTab);
                return [selectedTab, ...otherTabs];
            }
        }

        // Auto mode: use headset-based selection
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
    
    switchToGeneralTab() {
        const generalTab = this._tabs().find(tab => tab.type === 'General');
        if (generalTab) {
            this._selectedTab.set(generalTab);
        }
    }
    
    async enableDriver() {
        // For vendor-specific drivers, disable the neutral driver and enable the vendor driver
        if (vendor) {
            await this.sds.enableVendorDriverAndDisableNeutral();
        } else {
            await this.sds.enableSteamVRDriver(customHeadsetDriverName);
        }
    }

    async unblockAllDrivers() {
        await this.sds.unblockAllDrivers()
    }
}
