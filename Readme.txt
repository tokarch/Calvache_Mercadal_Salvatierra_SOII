GUÍA RÁPIDA GIT (EQUIPO DE 3) – COPIAR Y PEGAR

---

1. DESCARGAR EL PROYECTO (solo 1ª vez)

---

git clone https://github.com/USUARIO/NOMBRE_REPO.git
cd NOMBRE_REPO

---

2. ACTUALIZAR CAMBIOS DE LOS COMPAÑEROS
   (HACER SIEMPRE ANTES DE PROGRAMAR)

---

git pull origin main

(Si trabajáis con ramas personales:)
git fetch          # descarga todas las ramas nuevas

---

3. VER LAS RAMAS DISPONIBLES

---

git branch         # ramas locales
git branch -r      # ramas del remoto (compañeros)

---

4. CAMBIAR DE RAMA (TU RAMA)

---

git checkout tu-rama

Si no existe aún:
git checkout -b tu-rama

---

5. VER LA RAMA DE UN COMPAÑERO
   (EJ: lluis-rama)

---

Primera vez:
git fetch
git checkout -b lluis-rama origin/lluis-rama

Después (para actualizarla):
git checkout lluis-rama
git pull

---

6. TRABAJAR Y GUARDAR CAMBIOS EN TU ORDENADOR

---

git add .
git commit -m "Descripción clara del cambio"

---

7. SUBIR TUS CAMBIOS (A TU RAMA ONLINE QUE DIGAMOS)

---

Primera vez:
git push -u origin tu-rama

Después:
git push

---

8. FLUJO DIARIO RECOMENDADO (MEMORIZAR)

---

git pull (origin main)
git checkout toni-rama
(programar)
git add .
git commit -m "Qué has hecho"
git push

---

9. NORMAS IMPORTANTES EN EQUIPO

---

* NO trabajar directamente en main
* Cada persona usa su propia rama
* Hacer pull antes de empezar siempre
* No trabajar 2 personas en el mismo archivo a la vez
* Commits con mensajes claros (nada de "cosas" o "update")

Para subir cambios al main:
git checkout main
git pull origin main
git merge toni-rama
git push origin main
git checkout toni-rama

Descargarse cambios de main a tu rama:
git checkout main
git pull origin main
git checkout toni-rama
git merge main