import { inject } from "@angular/core";
import { PathsService } from "./services/paths.service";

export async function appInitializer() {
  const pathService = inject(PathsService);
  await pathService.ensureAllDirCreated();
  return 
}
