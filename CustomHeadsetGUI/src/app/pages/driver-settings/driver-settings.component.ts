import { Component, OnInit, OnDestroy, ChangeDetectionStrategy, inject } from '@angular/core';
import { SystemReadyComponent } from '../../utilities/system-ready/system-ready.component';
import { MeganexX8KComponent } from '../devices/meganex-x8-k/meganex-x8-k.component';
import { DreamAirComponent } from '../devices/dream-air/dream-air.component';
import { DreamAirSeComponent } from '../devices/dream-air-se/dream-air-se.component';
import { CrystalSuperMicroOledComponent } from '../devices/crystal-super-micro-oled/crystal-super-micro-oled.component';
import { CrystalSuper50PpdComponent } from '../devices/crystal-super-50ppd/crystal-super-50ppd.component';
import { CrystalSuper57PpdComponent } from '../devices/crystal-super-57ppd/crystal-super-57ppd.component';
import { CrystalSuperUltrawideComponent } from '../devices/crystal-super-ultrawide/crystal-super-ultrawide.component';
import { CrystalLightComponent } from '../devices/crystal-light/crystal-light.component';
import { CrystalOgComponent } from '../devices/crystal-og/crystal-og.component';
import { Pimax8KComponent } from '../devices/pimax-8k/pimax-8k.component';
import { Pimax5KComponent } from '../devices/pimax-5k/pimax-5k.component';
import { PimaxArtisanComponent } from '../devices/pimax-artisan/pimax-artisan.component';
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
  label: string;
  group?: string; // Optional group name for grouping tabs together
}

export interface TabGroup {
  name: string;
  tabs: TabConfig[];
}

@Component({
    selector: 'app-driver-settings',
    imports: [
        SystemReadyComponent,
        MeganexX8KComponent,
        DreamAirComponent,
        DreamAirSeComponent,
        CrystalSuperMicroOledComponent,
        CrystalSuper50PpdComponent,
        CrystalSuper57PpdComponent,
        CrystalSuperUltrawideComponent,
        CrystalLightComponent,
        CrystalOgComponent,
        Pimax8KComponent,
        Pimax5KComponent,
        PimaxArtisanComponent,
        GeneralComponent,
        MatTabsModule,
        MatIconModule,
        MatButtonModule,
        CommonModule
    ],
    providers: [MeganexX8KComponent, DreamAirComponent, DreamAirSeComponent, CrystalSuperMicroOledComponent, CrystalSuper50PpdComponent, CrystalSuper57PpdComponent, CrystalSuperUltrawideComponent, CrystalLightComponent, CrystalOgComponent, Pimax8KComponent, Pimax5KComponent, PimaxArtisanComponent],
    templateUrl: './driver-settings.component.html',
    styleUrl: './driver-settings.component.scss',
    changeDetection: ChangeDetectionStrategy.OnPush
})
export class DriverSettingsComponent implements OnInit, OnDestroy {
    // Tab configurations
    private _tabs = signal<TabConfig[]>([]);
    private _selectedTab = signal<TabConfig | undefined>(undefined);
    private _previousOrderedGroups: TabGroup[] = [];
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
    Pimax8KComponent = Pimax8KComponent;
    HeadsetType = HeadsetTypeEnum;

    constructor(private dis: DriverInfoService, private dss: DriverSettingService, private appSettingService: AppSettingService, public appUpdateService: AppUpdateService) {
        // Register tab configurations
        let availableTabs: TabConfig[] = [];
        availableTabs.push({ type: 'General', headsetType: HeadsetType.Other, label: $localize`General` })
        
        
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
            availableTabs.push({ type: 'MeganeX8K', headsetType: HeadsetType.MeganeX8K, label: $localize`MeganeX 8K` })
        }else{
            // disable when not shown
            onSettingsAvailable((settings) => {
                settings.meganeX8K.enable = false
                this.dss.save(settings)
            })
        }
        
        if(!vendorUi || vendorUi == "pimax"){
            availableTabs.push({ type: 'DreamAir', headsetType: HeadsetType.DreamAir, label: $localize`Dream Air`, group: $localize`Dream Air` })
            availableTabs.push({ type: 'DreamAirSE', headsetType: HeadsetType.DreamAirSE, label: $localize`Dream Air SE`, group: $localize`Dream Air` })
            availableTabs.push({ type: 'CrystalSuperMicroOLED', headsetType: HeadsetType.CrystalSuperMicroOLED, label: $localize`Super Micro-OLED`, group: $localize`Crystal Series` })
            availableTabs.push({ type: 'CrystalSuper50PPD', headsetType: HeadsetType.CrystalSuper50PPD, label: $localize`Super 50PPD`, group: $localize`Crystal Series` })
            availableTabs.push({ type: 'CrystalSuper57PPD', headsetType: HeadsetType.CrystalSuper57PPD, label: $localize`Super 57PPD`, group: $localize`Crystal Series` })
            availableTabs.push({ type: 'CrystalSuperUltrawide', headsetType: HeadsetType.CrystalSuperUltrawide, label: $localize`Super Ultrawide`, group: $localize`Crystal Series` })
            availableTabs.push({ type: 'CrystalLight', headsetType: HeadsetType.CrystalLight, label: $localize`Crystal Light`, group: $localize`Crystal Series` })
            availableTabs.push({ type: 'CrystalOG', headsetType: HeadsetType.CrystalOG, label: $localize`Crystal OG`, group: $localize`Crystal Series` })
            availableTabs.push({ type: 'Pimax8K', headsetType: HeadsetType.Pimax8KX, label: $localize`Pimax 8K Series`, group: $localize`P2 Series` })
            availableTabs.push({ type: 'Pimax5K', headsetType: HeadsetType.Pimax5KPlus, label: $localize`Pimax 5K Series`, group: $localize`P2 Series` })
            availableTabs.push({ type: 'PimaxArtisan', headsetType: HeadsetType.PimaxArtisan, label: $localize`Pimax Artisan`, group: $localize`P2 Series` })
        }else{
            // disable when not shown
            onSettingsAvailable((settings) => {
                settings.dreamAir.enable = false
                settings.dreamAirSE.enable = false
                settings.crystalSuperMicroOLED.enable = false
                settings.crystalSuper50PPD.enable = false
                settings.crystalSuper57PPD.enable = false
                settings.crystalSuperUltrawide.enable = false
                settings.crystalLight.enable = false
                settings.crystalOG.enable = false
                settings.pimax5KPlus.enable = false
                settings.pimax8KX.enable = false
                settings.pimaxArtisan.enable = false
                this.dss.save(settings)
            })
        }
        
        this._tabs.set(availableTabs)
        
        // Effect to update current headset type when driver info changes
        effect(() => {
            const info = this.dis.values();
            if (info && info.nonNativeHeadsetFound) {
                this.nonNativeWarning.set(true)
            } else {
                this.nonNativeWarning.set(false)
            }
        });
        
        // Effect to handle tab reordering and selection when headset changes
        effect(() => {
            const groups = this.tabGroups;
            const currentType = this.dis.currentHeadsetType();
            const selected = this._selectedTab();

            // Check if order changed
            const orderChanged = !this.groupsEqual(groups, this._previousOrderedGroups);

            setTimeout(() => {
                if (orderChanged) {
                    // If order changed, select the first tab of the first group
                    if (groups.length > 0 && groups[0].tabs.length > 0) {
                        this._selectedTab.set(groups[0].tabs[0]);
                    }
                } else if (!selected && groups.length > 0 && groups[0].tabs.length > 0) {
                    // If no selected tab yet, select the first one
                    this._selectedTab.set(groups[0].tabs[0]);
                }
            }, 0);


            console.log('Tab reordering triggered:', {
                groups,
                currentType,
                selected,
                newSelected: this._selectedTab(),
                previous: this._previousOrderedGroups
            });


            // Update previous ordered groups
            this._previousOrderedGroups = groups.map(g => ({ ...g, tabs: [...g.tabs] }));
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

    // Build tab groups from the flat tabs list, preserving definition order
    get tabGroups(): TabGroup[] {
        const tabs = this._tabs();
        const currentType = this.dis.currentHeadsetType();
        const appSettings = this.appSettingService.values();
        const defaultTab = appSettings?.defaultSettingsTab ?? 'auto';

        // Build groups preserving definition order from availableTabs
        let groups: TabGroup[] = [];
        const groupIndexMap = new Map<string, number>(); // group name -> index in groups array

        for (const tab of tabs) {
            if (tab.group) {
                // Grouped tab - add to existing group or create new one
                let groupIdx = groupIndexMap.get(tab.group);
                if (groupIdx === undefined) {
                    groups.push({ name: tab.group, tabs: [tab] });
                    groupIndexMap.set(tab.group, groups.length - 1);
                } else {
                    groups[groupIdx].tabs.push(tab);
                }
            } else {
                // Standalone tab - create its own group
                groups.push({ name: tab.label, tabs: [tab] });
            }
        }

        // Handle default tab selection (not 'auto')
        if (defaultTab !== 'auto') {
            const selectedTab = tabs.find(tab => tab.type === defaultTab);
            if (selectedTab) {
                // Find which group contains this tab
                const targetGroupIndex = groups.findIndex(g => g.tabs.includes(selectedTab));
                if (targetGroupIndex > -1 && targetGroupIndex !== 0) {
                    // Move group to front
                    const [targetGroup] = groups.splice(targetGroupIndex, 1);
                    groups.unshift(targetGroup);
                    // Move tab to front within group
                    const tabIndex = targetGroup.tabs.indexOf(selectedTab);
                    if (tabIndex > -1 && tabIndex !== 0) {
                        const [targetTab] = targetGroup.tabs.splice(tabIndex, 1);
                        targetGroup.tabs.unshift(targetTab);
                    }
                }
                return groups;
            }
        }

        // Auto mode: use headset-based selection
        if (!currentType) {
            // When headset is unknown, don't reorder
            return groups;
        }

        // Find the tab that matches the current headset type
        const specificTab = tabs.find(tab => tab.headsetType === currentType);
        if (!specificTab) {
            // If no matching tab for current headset, use default order
            return groups;
        }

        // Find which group contains the specific tab
        const targetGroupIndex = groups.findIndex(g => g.tabs.includes(specificTab));
        if (targetGroupIndex === -1) {
            return groups;
        }

        // Move the target group to the front
        const [targetGroup] = groups.splice(targetGroupIndex, 1);
        groups.unshift(targetGroup);

        // Move the specific tab to the front within its group
        const tabIndex = targetGroup.tabs.indexOf(specificTab);
        if (tabIndex > -1) {
            const [targetTab] = targetGroup.tabs.splice(tabIndex, 1);
            targetGroup.tabs.unshift(targetTab);
        }

        return groups;
    }

    // Get the selected group index
    get selectedGroupIndex(): number {
        const selected = this._selectedTab();
        const groups = this.tabGroups;
        if (selected) {
            return groups.findIndex(g => g.tabs.includes(selected));
        }
        return 0;
    }

    // Get the selected tab index within the current group
    get selectedTabInGroupIndex(): number {
        const selected = this._selectedTab();
        const groups = this.tabGroups;
        const groupIndex = this.selectedGroupIndex;
        if (selected && groupIndex >= 0 && groupIndex < groups.length) {
            return groups[groupIndex].tabs.indexOf(selected);
        }
        return 0;
    }

    // Handle outer group tab change
    onGroupChange(index: number): void {
        const groups = this.tabGroups;
        if (groups[index]) {
            // Select the first tab in the new group
            if (groups[index].tabs.length > 0) {
                this._selectedTab.set(groups[index].tabs[0]);
            }
        }
    }

    // Handle inner tab change within a group
    onTabInGroupChange(index: number): void {
        const groups = this.tabGroups;
        const groupIndex = this.selectedGroupIndex;
        if (groupIndex >= 0 && groups[groupIndex] && groups[groupIndex].tabs[index]) {
            this._selectedTab.set(groups[groupIndex].tabs[index]);
        }
    }

    // Check if a group is a standalone (single tab with no group property)
    isStandaloneGroup(group: TabGroup): boolean {
        return !group.tabs[0].group;
    }

    // Compare two arrays of groups for equality
    private groupsEqual(a: TabGroup[], b: TabGroup[]): boolean {
        if (a.length !== b.length) return false;
        for (let i = 0; i < a.length; i++) {
            if (a[i].name !== b[i].name) return false;
            if (a[i].tabs.length !== b[i].tabs.length) return false;
            for (let j = 0; j < a[i].tabs.length; j++) {
                if (a[i].tabs[j] !== b[i].tabs[j]) return false;
            }
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