describe("My First Test", () => {
  it("Visits the app root url", () => {
    cy.visit("/");
    cy.log("Hello");
    cy.contains("#container", "Hello");
  });
});
