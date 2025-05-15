import { inject } from "@angular/core";
import { PathService } from "./services/path.service";

export async function appInitializer() {
  const pathService = inject(PathService);
  await pathService.ensureAllDirCreated();
  return 
}
