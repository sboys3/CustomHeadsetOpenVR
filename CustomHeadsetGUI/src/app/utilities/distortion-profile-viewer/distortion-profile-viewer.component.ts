import { Component, effect, ElementRef, input, viewChild, signal, OnDestroy } from '@angular/core';
import { PathsService } from '../../services/paths.service';
import { exists, readTextFile } from '@tauri-apps/plugin-fs';
import { cleanJsonComments } from '../../helpers';
import { DistortionProfileConfig } from '../../services/JsonFileDefines';

@Component({
  selector: 'app-distortion-profile-viewer',
  templateUrl: './distortion-profile-viewer.component.html',
  styleUrls: ['./distortion-profile-viewer.component.scss']
})
export class DistortionProfileViewerComponent implements OnDestroy {
  meganeXDefault = [
    [0.00000, 0.0],
    [5.00000, 12.463],
    [10.0000, 24.750],
    [15.00000, 36.665],
    [20.00000, 48.103],
    [25.00000, 59.093],
    [30.00000, 69.749],
    [35.00000, 79.994],
    [40.00000, 89.147],
    [45.00000, 96.357],
    [48.30730, 100.0]
  ];

  // meganeXOriginal = [
  //   [0.0, 0.0],
  //   [10.0, 24.77952472],
  //   [20.0, 48.32328161],
  //   [30.0, 69.9136628],
  //   [35.0, 79.99462488],
  //   [40.0, 89.06057112],
  //   [45.0, 96.29634484],
  //   [48.3073, 100.0],
  // ];
  profiles = input<string[]>()
  windowSize = signal<{ width: number, height: number } | undefined>(undefined);
  canvasRef = viewChild.required<ElementRef<HTMLCanvasElement>>('canvas');

  private resizeObserver: ResizeObserver;
  constructor(private pathsService: PathsService, host: ElementRef<HTMLElement>) {
    let width = 0, height = 0;
    const colors = ['green', 'orange', 'blueviolet', 'deeppink']
    const profileData = signal<DistortionProfileConfig[]>([]);
    this.resizeObserver = new ResizeObserver(() => {
      this.windowSize.set({ width: window.innerWidth, height: window.innerHeight });
    })
    this.resizeObserver.observe(host.nativeElement)
    effect(async () => {
      const profiles = this.profiles()
      let newData: DistortionProfileConfig[] = []
      if (profiles) {
        for (const profile of profiles) {
          const path = await this.pathsService.getProfileFullPath(`${profile}.json`);
          try {
            if (await exists(path)) {
              const content = await readTextFile(path);
              const obj = JSON.parse(cleanJsonComments(content)) as DistortionProfileConfig
              if (obj.distortions.length % 2 == 0) {
                obj.name = profile;
                newData.push(obj)
              }
            }
          } catch (e) {
            console.warn(e)
          }
        }
      }
      profileData.set(newData);
    });
    effect(() => {
      let curveIdx = 0;
      const canvas = this.canvasRef().nativeElement;
      // resize element when window resizes
      let windowSize = this.windowSize();
      //delay first draw so we can get real window size
      if (!windowSize) return;
      const b = host.nativeElement.getBoundingClientRect()
      width = b.width - 1;
      height = Math.min(b.height - 20, windowSize.height * 0.8);
      canvas.width = width;
      canvas.height = height;
      const ctx = canvas.getContext('2d') as CanvasRenderingContext2D;
      this.drawPoints(canvas, ctx, this.smoothPoints(this.meganeXDefault, 20), 'red', 'meganeX Default', curveIdx);
      curveIdx++;
      // this.drawPoints(canvas, ctx, this.smoothPoints(this.meganeXOriginal, 20), 'blue', 'meganeX Original', curveIdx);
      // curveIdx++;
      const data = profileData();
      for (const obj of data) {
        this.drawPoints(canvas, ctx, this.smoothPoints(this.chunkArrayInPairs(obj.distortions), 20), colors[(curveIdx - 2) % colors.length], obj.name, curveIdx);
        curveIdx++;
      }
    });

  }
  ngOnDestroy(): void {
    this.resizeObserver.disconnect()
  }
  chunkArrayInPairs(arr: number[]): number[][] {
    const result: number[][] = [];
    for (let i = 0; i < arr.length; i += 2) {
      const pair: number[] = [arr[i]];
      if (i + 1 < arr.length) {
        pair.push(arr[i + 1]);
      }
      result.push(pair);
    }
    return result;
  }
  private drawPoints(
    canvas: HTMLCanvasElement,
    ctx: CanvasRenderingContext2D,
    points: number[][],
    color: string,
    name: string,
    curveIdx: number
  ): void {
    ctx.lineWidth = 1;

    const canvasWidth = canvas.width;
    const canvasHeight = canvas.height;
    const scaledCanvasWidth = canvasWidth / 120;
    const scaledCanvasHeight = canvasHeight / 120;
    const canvasX = scaledCanvasWidth * 10;
    const canvasY = scaledCanvasHeight * 10;

    ctx.strokeStyle = 'black';
    ctx.strokeRect(
      canvasX,
      canvasY,
      scaledCanvasWidth * 100,
      scaledCanvasHeight * 100
    );

    ctx.beginPath();
    ctx.strokeStyle = color;
    const pointsCanvasCoords = points.map(([x, y]) => [
      x * scaledCanvasWidth + canvasX,
      canvasHeight - y * scaledCanvasHeight - canvasY
    ]);

    ctx.moveTo(pointsCanvasCoords[0][0], pointsCanvasCoords[0][1]);
    for (let i = 1; i < pointsCanvasCoords.length; i++) {
      ctx.lineTo(pointsCanvasCoords[i][0], pointsCanvasCoords[i][1]);
    }
    ctx.stroke();
    ctx.fillStyle = color;
    const y = scaledCanvasHeight * (105 - (curveIdx * 6));
    ctx.strokeStyle = 'black'
    ctx.fillRect(scaledCanvasWidth * 50, y, 15, 15)
    ctx.strokeRect(scaledCanvasWidth * 50, y, 15, 15)
    ctx.textBaseline = 'middle'
    ctx.font = '16px Roboto'
    this.drawStrokedText(ctx, name, (scaledCanvasWidth * 50 + 20), y + 7.5)

  }
  private drawStrokedText(ctx: CanvasRenderingContext2D, text: string, x: number, y: number) {
    // using the solutions from @Simon Sarris and @Jackalope from
    // https://stackoverflow.com/questions/7814398/a-glow-effect-on-html5-canvas
    ctx.save();
    ctx.strokeStyle = 'black';
    ctx.lineWidth = 4;
    ctx.lineJoin = "round";
    ctx.miterLimit = 2;
    ctx.strokeText(text, x, y);
    ctx.fillText(text, x, y);
    ctx.restore();
  }

  private bezierPoint(t: number, controlPoints: number[][]): number[] {
    let [P0, P1, P2, P3] = controlPoints;
    let tSquared = t * t;
    let oneMinusT = 1 - t;
    let oneMinusTSquared = oneMinusT * oneMinusT;

    let pointX = (
      Math.pow(oneMinusT, 3) * P0[0] +
      3 * oneMinusTSquared * t * P1[0] +
      3 * oneMinusT * tSquared * P2[0] +
      Math.pow(t, 3) * P3[0]
    );

    let pointY = (
      Math.pow(oneMinusT, 3) * P0[1] +
      3 * oneMinusTSquared * t * P1[1] +
      3 * oneMinusT * tSquared * P2[1] +
      Math.pow(t, 3) * P3[1]
    );

    return [pointX, pointY];
  }

  private smoothPoints(points: number[][], innerPointCounts: number): number[][] {
    // how far out to move the center bezier points from the existing points
    // larger values will make the curve more "smooth" and less "sharp" at the existing points
    let smoothAmount = 1 / 3
    let outPoints = []
    for (let i = 0; i < points.length - 1; i++) {
      // the new points will be inserted between existing points
      let prevPoint = points[i]
      let nextPoint = points[i + 1]
      let prevPrevPoint = points[i - 1]
      let nextNextPoint = points[i + 2]
      // find slope for prev and next point based on the points that surround them
      let fallbackSlope = (nextPoint[1] - prevPoint[1]) / (nextPoint[0] - prevPoint[0])
      let prevSlope = i == 0 ? fallbackSlope : (nextPoint[1] - prevPrevPoint[1]) / (nextPoint[0] - prevPrevPoint[0])
      fallbackSlope = (nextPoint[1] - prevPoint[1]) / (nextPoint[0] - prevPoint[0])
      let nextSlope = (i >= points.length - 2) ? fallbackSlope : (nextNextPoint[1] - prevPoint[1]) / (nextNextPoint[0] - prevPoint[0])
      // extrapolate center points based on the slopes
      let centerDistance = (nextPoint[0] - prevPoint[0]) * smoothAmount
      let centerFromPrev = centerDistance * prevSlope + prevPoint[1]
      let centerFromNext = -centerDistance * nextSlope + nextPoint[1]

      // create a bezier curve with the extrapolated center points and the existing points as anchors
      let controlPoints = [
        prevPoint,
        [prevPoint[0] + centerDistance, centerFromPrev],
        [nextPoint[0] - centerDistance, centerFromNext],
        nextPoint
      ]
      // console.log(i, controlPointsExplicit)

      outPoints.push(prevPoint)
      // generate inner points based on the bezier curve
      for (let j = 0; j < innerPointCounts; j++) {
        outPoints.push(this.bezierPoint((j + 1) / (innerPointCounts + 1), controlPoints))
      }
    }
    outPoints.push(points[points.length - 1])
    return outPoints
  }
}
